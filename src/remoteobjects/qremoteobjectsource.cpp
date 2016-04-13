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

#include "qconnectionfactories.h"
#include "qremoteobjectsourceio_p.h"

#include <QMetaProperty>
#include <QVarLengthArray>

#include <algorithm>
#include <iterator>

QT_BEGIN_NAMESPACE

using namespace QRemoteObjectPackets;

const int QRemoteObjectSource::qobjectPropertyOffset = QObject::staticMetaObject.propertyCount();
const int QRemoteObjectSource::qobjectMethodOffset = QObject::staticMetaObject.methodCount();

QRemoteObjectSource::QRemoteObjectSource(QObject *obj, const SourceApiMap *api,
                                         QObject *adapter, QRemoteObjectSourceIoAbstract *sourceIo)
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
            if (!QMetaObject::connect(adapter, sourceIndex, this, QRemoteObjectSource::qobjectMethodOffset+idx, Qt::DirectConnection, 0)) {
                qCWarning(QT_REMOTEOBJECT) << "QRemoteObjectSourcePrivate: QMetaObject::connect returned false. Unable to connect.";
                return;
            }
        } else {
            if (!QMetaObject::connect(obj, sourceIndex, this, QRemoteObjectSource::qobjectMethodOffset+idx, Qt::DirectConnection, 0)) {
                qCWarning(QT_REMOTEOBJECT) << "QRemoteObjectSourcePrivate: QMetaObject::connect returned false. Unable to connect.";
                return;
            }
        }

        qCDebug(QT_REMOTEOBJECT) << "Connection made" << idx << meta->method(sourceIndex).name();
    }

    m_sourceIo->registerSource(this);
}

QRemoteObjectSource::~QRemoteObjectSource()
{
    m_sourceIo->unregisterSource(this);
    Q_FOREACH (ServerIoDevice *io, listeners) {
        removeListener(io, true);
    }
    delete m_api;
}

QVariantList* QRemoteObjectSource::marshalArgs(int index, void **a)
{
    QVariantList &list = m_marshalledArgs;
    const int N = m_api->signalParameterCount(index);
    if (list.size() < N)
        list.reserve(N);
    const int minFill = std::min(list.size(), N);
    for (int i = 0; i < minFill; ++i) {
        const int type = m_api->signalParameterType(index, i);
        if (type == QMetaType::QVariant)
            list[i] = *reinterpret_cast<QVariant *>(a[i + 1]);
        else
            list[i] = QVariant(type, a[i + 1]);
    }
    for (int i = list.size(); i < N; ++i) {
        const int type = m_api->signalParameterType(index, i);
        if (type == QMetaType::QVariant)
            list << *reinterpret_cast<QVariant *>(a[i + 1]);
        else
            list << QVariant(type, a[i + 1]);
    }
    for (int i = N; i < list.size(); ++i)
        list.removeLast();
    return &m_marshalledArgs;
}

bool QRemoteObjectSource::invoke(QMetaObject::Call c, bool forAdapter, int index, const QVariantList &args, QVariant* returnValue)
{
    int status = -1;
    int flags = 0;

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
    } else if (c == QMetaObject::WriteProperty) {
        for (int i = 0; i < args.size(); ++i) {
            param[i] = const_cast<void*>(args.at(i).data());
        }
        Q_ASSERT(param.size() == 2); // for return-value and setter value
        // check QMetaProperty::write for an explanation of these
        param.append(&status);
        param.append(&flags);
    } else {
        for (int i = 0; i < args.size(); ++i) {
            param[i] = const_cast<void*>(args.at(i).data());
        }
    }
    int r = -1;
    if (forAdapter)
        r = m_adapter->qt_metacall(c, index, param.data());
    else
        r = parent()->qt_metacall(c, index, param.data());
    return r == -1 && status == -1;
}

void QRemoteObjectSource::handleMetaCall(int index, QMetaObject::Call call, void **a)
{
    if (listeners.empty())
        return;

    const int propertyIndex = m_api->propertyIndexFromSignal(index);
    if (propertyIndex >= 0) {
        const int rawIndex = m_api->propertyRawIndexFromSignal(index);
        if (m_api->isAdapterProperty(index)) {
            const QMetaProperty mp = m_adapter->metaObject()->property(propertyIndex);
            qCDebug(QT_REMOTEOBJECT) << "Sending Invoke Property (adapter)" << rawIndex << propertyIndex << mp.name() << mp.read(m_adapter);
            serializePropertyChangePacket(m_packet, m_api->name(), rawIndex, serializedProperty(mp, m_adapter));
        } else {
            const QMetaProperty mp = m_object->metaObject()->property(propertyIndex);
            qCDebug(QT_REMOTEOBJECT) << "Sending Invoke Property" << rawIndex << propertyIndex << mp.name() << mp.read(m_object);
            serializePropertyChangePacket(m_packet, m_api->name(), rawIndex, serializedProperty(mp, m_object));
        }
        m_packet.baseAddress = m_packet.size;
    }

    qCDebug(QT_REMOTEOBJECT) << "# Listeners" << listeners.length();
    qCDebug(QT_REMOTEOBJECT) << "Invoke args:" << m_object << call << index << marshalArgs(index, a);

    serializeInvokePacket(m_packet, m_api->name(), call, index, *marshalArgs(index, a));
    m_packet.baseAddress = 0;

    Q_FOREACH (ServerIoDevice *io, listeners)
        io->write(m_packet.array, m_packet.size);
}

void QRemoteObjectSource::addListener(ServerIoDevice *io, bool dynamic)
{
    listeners.append(io);

    if (dynamic) {
        serializeInitDynamicPacket(m_packet, this);
        io->write(m_packet.array, m_packet.size);
    } else {
        serializeInitPacket(m_packet, this);
        io->write(m_packet.array, m_packet.size);
    }
}

int QRemoteObjectSource::removeListener(ServerIoDevice *io, bool shouldSendRemove)
{
    listeners.removeAll(io);
    if (shouldSendRemove)
    {
        serializeRemoveObjectPacket(m_packet, m_api->name());
        io->write(m_packet.array, m_packet.size);
    }
    return listeners.length();
}

int QRemoteObjectSource::qt_metacall(QMetaObject::Call call, int methodId, void **a)
{
    methodId = QObject::qt_metacall(call, methodId, a);
    if (methodId < 0)
        return methodId;

    if (call == QMetaObject::InvokeMetaMethod)
        handleMetaCall(methodId, call, a);

    return -1;
}

DynamicApiMap::DynamicApiMap(const QMetaObject *metaObject, const QString &name, const QString &typeName)
    : m_name(name),
      m_typeName(typeName),
      m_metaObject(metaObject),
      m_cachedMetamethodIndex(-1)
{
    const int propCount = metaObject->propertyCount();
    const int propOffset = metaObject->propertyOffset();
    m_properties.reserve(propCount-propOffset);
    int i = 0;
    for (i = propOffset; i < propCount; ++i) {
        m_properties << i;
        const int notifyIndex = metaObject->property(i).notifySignalIndex();
        if (notifyIndex != -1) {
            m_signals << notifyIndex;
            m_propertyAssociatedWithSignal.append(i-propOffset);
            //The starting values of _signals will be the notify signals
            //So if we are processing _signal with index i, api->sourcePropertyIndex(_propertyAssociatedWithSignal.at(i))
            //will be the property that changed.  This is only valid if i < _propertyAssociatedWithSignal.size().
        }
    }
    const int methodCount = metaObject->methodCount();
    const int methodOffset = metaObject->methodOffset();
    for (i = methodOffset; i < methodCount; ++i) {
        const QMetaMethod mm = metaObject->method(i);
        const QMetaMethod::MethodType m = mm.methodType();
        if (m == QMetaMethod::Signal) {
            if (m_signals.indexOf(i) >= 0) //Already added as a property notifier
                continue;
            m_signals << i;
        } else if (m == QMetaMethod::Slot || m == QMetaMethod::Method)
            m_methods << i;
    }
}

int DynamicApiMap::parameterCount(int objectIndex) const
{
    checkCache(objectIndex);
    return m_cachedMetamethod.parameterCount();
}

int DynamicApiMap::parameterType(int objectIndex, int paramIndex) const
{
    checkCache(objectIndex);
    return m_cachedMetamethod.parameterType(paramIndex);
}

const QByteArray DynamicApiMap::signature(int objectIndex) const
{
    checkCache(objectIndex);
    return m_cachedMetamethod.methodSignature();
}

QMetaMethod::MethodType DynamicApiMap::methodType(int index) const
{
    const int objectIndex = m_methods.at(index);
    checkCache(objectIndex);
    return m_cachedMetamethod.methodType();
}

const QByteArray DynamicApiMap::typeName(int index) const
{
    const int objectIndex = m_methods.at(index);
    checkCache(objectIndex);
    return m_cachedMetamethod.typeName();
}

QT_END_NAMESPACE
