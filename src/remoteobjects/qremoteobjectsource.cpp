/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qremoteobjectsource.h"
#include "qremoteobjectsource_p.h"

#include "qconnectionabstractserver_p.h"
#include "qremoteobjectpacket_p.h"
#include "qtremoteobjectglobal.h"
#include "qremoteobjectsourceio_p.h"

#include <QMetaProperty>
#include <QVarLengthArray>

#include <algorithm>
#include <iterator>

QT_BEGIN_NAMESPACE

using namespace QRemoteObjectPackets;

const int QRemoteObjectSourcePrivate::qobjectPropertyOffset = QObject::staticMetaObject.propertyCount();
const int QRemoteObjectSourcePrivate::qobjectMethodOffset = QObject::staticMetaObject.methodCount();

QRemoteObjectSourcePrivate::QRemoteObjectSourcePrivate(QObject *obj, const SourceApiMap *api,
                                                       QObject *adapter, QRemoteObjectSourceIo *sourceIo)
    : QObject(obj),
      m_object(obj),
      m_adapter(adapter),
      m_api(api),
      m_sourceIo(sourceIo)
{
    if (!obj) {
        qCWarning(QT_REMOTEOBJECT) << "QRemoteObjectSourcePrivate: Cannot replicate a NULL object" << m_api->name();
        return;
    }

    const QMetaObject *meta = obj->metaObject();
    for (int idx = 0; idx < m_api->signalCount(); ++idx) {
        const int sourceIndex = m_api->sourceSignalIndex(idx);

        // This basically connects the parent Signals (note, all dynamic properties have onChange
        //notifications, thus signals) to us.  Normally each Signal is mapped to a unique index,
        //but since we are forwarding them all, we keep the offset constant.
        //
        //We know no one will inherit from this class, so no need to worry about indices from
        //derived classes.
        if (m_api->isAdapterSignal(idx)) {
            if (!QMetaObject::connect(adapter, sourceIndex, this, QRemoteObjectSourcePrivate::qobjectMethodOffset+idx, Qt::DirectConnection, 0)) {
                qCWarning(QT_REMOTEOBJECT) << "QRemoteObjectSourcePrivate: QMetaObject::connect returned false. Unable to connect.";
                return;
            }
        } else {
            if (!QMetaObject::connect(obj, sourceIndex, this, QRemoteObjectSourcePrivate::qobjectMethodOffset+idx, Qt::DirectConnection, 0)) {
                qCWarning(QT_REMOTEOBJECT) << "QRemoteObjectSourcePrivate: QMetaObject::connect returned false. Unable to connect.";
                return;
            }
        }

        qCDebug(QT_REMOTEOBJECT) << "Connection made" << idx << meta->method(sourceIndex).name();
    }

    m_sourceIo->registerSource(this);
}

QRemoteObjectSourcePrivate::~QRemoteObjectSourcePrivate()
{
    m_sourceIo->unregisterSource(this);
    Q_FOREACH (ServerIoDevice *io, listeners) {
        removeListener(io, true);
    }
    delete m_api;
}

QVariantList QRemoteObjectSourcePrivate::marshalArgs(int index, void **a)
{
    QVariantList list;
    const int N = m_api->signalParameterCount(index);
    list.reserve(N);
    for (int i = 0; i < N; ++i) {
        const int type = m_api->signalParameterType(index, i);
        if (type == QMetaType::QVariant)
            list << *reinterpret_cast<QVariant *>(a[i + 1]);
        else
            list << QVariant(type, a[i + 1]);
    }
    return list;
}

bool QRemoteObjectSourcePrivate::invoke(QMetaObject::Call c, bool forAdapter, int index, const QVariantList &args, QVariant* returnValue)
{
    QVarLengthArray<void*, 10> param(args.size() + 1);

    if (c == QMetaObject::InvokeMetaMethod) {
        if (returnValue) {
            param[0] = returnValue->data();
        } else {
            param[0] = Q_NULLPTR;
        }

        for (int i = 0; i < args.size(); ++i) {
            param[i + 1] = const_cast<void*>(args.at(i).data());
        }

    } else {
        for (int i = 0; i < args.size(); ++i) {
            param[i] = const_cast<void*>(args.at(i).data());
        }

    }
    if (forAdapter)
        return (m_adapter->qt_metacall(c, index, param.data()) == -1);
    return (parent()->qt_metacall(c, index, param.data()) == -1);
}

#ifdef Q_COMPILER_UNIFORM_INIT
// QPair (like any class) can be initialized with { }
typedef QPair<QString,QVariant> Pair;
inline Pair make_pair(QString first, QVariant second)
{ return { qMove(first), qMove(second) }; }
#else
// QPair can't be initialized with { }, need to use a POD
struct Pair {
    QString first;
    QVariant second;
};
inline QDataStream &operator<<(QDataStream &s, const Pair &p)
{ return s << p.first << p.second; }
inline Pair make_pair(QString first, QVariant second)
{ Pair p = { qMove(first), qMove(second) }; return p; }
#endif

void QRemoteObjectSourcePrivate::handleMetaCall(int index, QMetaObject::Call call, void **a)
{
    if (listeners.empty())
        return;

    QByteArray ba;

    const int propertyIndex = m_api->propertyIndexFromSignal(index);
    if (propertyIndex >= 0) {
        if (m_api->isAdapterProperty(index)) {
            const QMetaProperty mp = m_adapter->metaObject()->property(propertyIndex);
            qCDebug(QT_REMOTEOBJECT) << "Invoke Property (adapter)" << propertyIndex << mp.name() << mp.read(m_adapter);
            QPropertyChangePacket p(m_api->name(), mp.name(), mp.read(m_adapter));
            ba = p.serialize();
        } else {
            const QMetaProperty mp = m_object->metaObject()->property(propertyIndex);
            qCDebug(QT_REMOTEOBJECT) << "Invoke Property" << propertyIndex << mp.name() << mp.read(m_object);
            QPropertyChangePacket p(m_api->name(), mp.name(), mp.read(m_object));
            ba = p.serialize();
        }
    }

    qCDebug(QT_REMOTEOBJECT) << "# Listeners" << listeners.length();
    qCDebug(QT_REMOTEOBJECT) << "Invoke args:" << m_object << call << index << marshalArgs(index, a);
    QInvokePacket p(m_api->name(), call, index, marshalArgs(index, a));

    ba += p.serialize();

    Q_FOREACH (ServerIoDevice *io, listeners)
        io->write(ba);
}

void QRemoteObjectSourcePrivate::addListener(ServerIoDevice *io, bool dynamic)
{
    listeners.append(io);

    if (dynamic) {
        QRemoteObjectPackets::QInitDynamicPacketEncoder p(this);
        io->write(p.serialize());
    } else {
        QRemoteObjectPackets::QInitPacketEncoder p(this);
        io->write(p.serialize());
    }
}

int QRemoteObjectSourcePrivate::removeListener(ServerIoDevice *io, bool shouldSendRemove)
{
    listeners.removeAll(io);
    if (shouldSendRemove)
    {
        QRemoveObjectPacket p(m_api->name());
        io->write(p.serialize());
    }
    return listeners.length();
}

int QRemoteObjectSourcePrivate::qt_metacall(QMetaObject::Call call, int methodId, void **a)
{
    methodId = QObject::qt_metacall(call, methodId, a);
    if (methodId < 0)
        return methodId;

    if (call == QMetaObject::InvokeMetaMethod)
        handleMetaCall(methodId, call, a);

    return -1;
}

DynamicApiMap::DynamicApiMap(const QMetaObject *meta, const QString &name)
    : _name(name),
      _meta(meta),
      _cachedMetamethodIndex(-1)
{
    const int propCount = meta->propertyCount();
    const int propOffset = meta->propertyOffset();
    _properties.reserve(propCount-propOffset);
    int i = 0;
    for (i = propOffset; i < propCount; ++i) {
        _properties << i;
        const int notifyIndex = meta->property(i).notifySignalIndex();
        if (notifyIndex != -1) {
            _signals << notifyIndex;
            _propertyAssociatedWithSignal.append(i);
            //The starting values of _signals will be the notify signals
            //So if we are processing _signal with index i, api->sourcePropertyIndex(_propertyAssociatedWithSignal.at(i))
            //will be the property that changed.  This is only valid if i < _propertyAssociatedWithSignal.size().
        }
    }
    const int methodCount = meta->methodCount();
    const int methodOffset = meta->methodOffset();
    for (i = methodOffset; i < methodCount; ++i) {
        const QMetaMethod mm = meta->method(i);
        const QMetaMethod::MethodType m = mm.methodType();
        if (m == QMetaMethod::Signal) {
            if (_signals.indexOf(i) >= 0) //Already added as a property notifier
                continue;
            _signals << i;
        } else if (m == QMetaMethod::Slot || m == QMetaMethod::Method)
            _methods << i;
    }
}

int DynamicApiMap::parameterCount(int objectIndex) const
{
    checkCache(objectIndex);
    return _cachedMetamethod.parameterCount();
}

int DynamicApiMap::parameterType(int objectIndex, int paramIndex) const
{
    checkCache(objectIndex);
    return _cachedMetamethod.parameterType(paramIndex);
}

const QByteArray DynamicApiMap::signature(int objectIndex) const
{
    checkCache(objectIndex);
    return _cachedMetamethod.methodSignature();
}

QMetaMethod::MethodType DynamicApiMap::methodType(int index) const
{
    const int objectIndex = _methods.at(index);
    checkCache(objectIndex);
    return _cachedMetamethod.methodType();
}

const QByteArray DynamicApiMap::typeName(int index) const
{
    const int objectIndex = _methods.at(index);
    checkCache(objectIndex);
    return _cachedMetamethod.typeName();
}

QT_END_NAMESPACE
