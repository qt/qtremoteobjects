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

#include "qremoteobjectsource_p.h"

#include "qconnectionabstractserver_p.h"
#include "qremoteobjectpacket_p.h"
#include "qtremoteobjectglobal.h"

#include <QMetaProperty>
#include <QVarLengthArray>

#include <algorithm>
#include <iterator>

QT_BEGIN_NAMESPACE

using namespace QRemoteObjectPackets;

static QVector<int> parameter_types(const QMetaMethod &member)
{
    QVector<int> types;
    types.reserve(member.parameterCount());
    for (int i = 0; i < member.parameterCount(); ++i) {
        const int tp = member.parameterType(i);
        if (tp == QMetaType::UnknownType) {
            Q_ASSERT(tp != QMetaType::Void); // void parameter => metaobject is corrupt
            qCWarning(QT_REMOTEOBJECT) <<"Don't know how to handle "
                                    << member.parameterTypes().at(i).constData()
                                    << ", use qRegisterMetaType to register it.";

        }
        types << tp;
    }
    return types;
}

QRemoteObjectSourcePrivate::QRemoteObjectSourcePrivate(QObject *obj, QMetaObject const *meta, const QString &name)
    : QObject(obj),
      args(meta->methodCount()),
      m_name(name),
      m_object(obj),
      m_meta(meta),
      m_methodOffset(meta->methodCount()),
      m_propertyOffset(meta->propertyCount())
{
    if (!obj) {
        qCWarning(QT_REMOTEOBJECT) << "QRemoteObjectSourcePrivate: Cannot replicate a NULL object" << name;
        return;
    }

    const QMetaObject *them = obj->metaObject();
    for (int idx = m_propertyOffset; idx < them->propertyCount(); ++idx) {
        const QMetaProperty mp = them->property(idx);
        qCDebug(QT_REMOTEOBJECT) << "Property option" << idx << mp.name() << mp.isWritable() << mp.hasNotifySignal() << mp.notifySignalIndex();
        if (mp.hasNotifySignal())
            propertyFromNotifyIndex.insert(mp.notifySignalIndex(), mp);
    }

    args.reserve(them->methodCount() - m_methodOffset);

    for (int idx = m_methodOffset; idx < them->methodCount(); ++idx) {
        const QMetaMethod mm = them->method(idx);
        qCDebug(QT_REMOTEOBJECT) << "Connection option" << idx << mm.name();

        if (mm.methodType() != QMetaMethod::Signal)
            continue;

        // This basically connects the parent Signals (note, all dynamic properties have onChange
        //notifications, thus signals) to us.  Normally each Signal is mapped to a unique index,
        //but since we are forwarding them all, we keep the offset constant.
        //
        //We know no one will inherit from this class, so no need to worry about indices from
        //derived classes.
        if (!QMetaObject::connect(obj, idx, this, m_methodOffset, Qt::DirectConnection, 0)) {
            qCWarning(QT_REMOTEOBJECT) << "QRemoteObjectSourcePrivate: QMetaObject::connect returned false. Unable to connect.";
            return;
        }

        args.push_back(parameter_types(mm));

        qCDebug(QT_REMOTEOBJECT) << "Connection made" << idx << mm.name();
    }
}

QRemoteObjectSourcePrivate::~QRemoteObjectSourcePrivate()
{
    Q_FOREACH (ServerIoDevice *io, listeners) {
        removeListener(io, true);
    }
}

QVariantList QRemoteObjectSourcePrivate::marshalArgs(int index, void **a)
{
    QVariantList list;
    const QVector<int> &argsForIndex = args[index];
    const int N = argsForIndex.size();
    list.reserve(N);
    for (int i = 0; i < N; ++i) {
        const int type = argsForIndex[i];
        if (type == QMetaType::QVariant)
            list << *reinterpret_cast<QVariant *>(a[i + 1]);
        else
            list << QVariant(type, a[i + 1]);
    }
    return list;
}

bool QRemoteObjectSourcePrivate::invoke(QMetaObject::Call c, int index, const QVariantList &args, QVariant* returnValue)
{
    QVarLengthArray<void*, 10> param(args.size() + 1);

    if (c == QMetaObject::InvokeMetaMethod) {
        if (returnValue) {
            const QMetaMethod metaMethod = parent()->metaObject()->method(index + m_methodOffset);
            int typeId = metaMethod.returnType();
            if (!QMetaType(typeId).sizeOf())
                typeId = QVariant::Invalid;
            QVariant tmp(typeId, Q_NULLPTR);
            returnValue->swap(tmp);
            param[0] = returnValue->data();
        } else {
            param[0] = Q_NULLPTR;
        }

        for (int i = 0; i < args.size(); ++i) {
            param[i + 1] = const_cast<void*>(args.at(i).data());
        }

        return (parent()->qt_metacall(c, index + m_methodOffset, param.data()) == -1);
    } else {
        for (int i = 0; i < args.size(); ++i) {
            param[i] = const_cast<void*>(args.at(i).data());
        }

        return (parent()->qt_metacall(c, index + m_propertyOffset, param.data()) == -1);
    }
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

    if (propertyFromNotifyIndex.contains(index)) {
        const QMetaProperty mp = propertyFromNotifyIndex[index];
        qCDebug(QT_REMOTEOBJECT) << "Invoke Property" << mp.name() << mp.read(m_object);
        QPropertyChangePacket p(m_name, mp.name(), mp.read(m_object));
        ba = p.serialize();
    }

    qCDebug(QT_REMOTEOBJECT) << "# Listeners" << listeners.length();
    qCDebug(QT_REMOTEOBJECT) << "Invoke args:" << m_object << call << index << marshalArgs(index, a);
    QInvokePacket p(m_name, call, index - m_methodOffset, marshalArgs(index, a));

    ba += p.serialize();

    Q_FOREACH (ServerIoDevice *io, listeners)
        io->write(ba);
}

void QRemoteObjectSourcePrivate::addListener(ServerIoDevice *io, bool dynamic)
{
    listeners.append(io);

    if (dynamic) {
        QRemoteObjectPackets::QInitDynamicPacketEncoder p(m_name, m_object, m_meta);
        io->write(p.serialize());
    } else {
        QRemoteObjectPackets::QInitPacketEncoder p(m_name, m_object, m_meta);
        io->write(p.serialize());
    }
}

int QRemoteObjectSourcePrivate::removeListener(ServerIoDevice *io, bool shouldSendRemove)
{
    listeners.removeAll(io);
    if (shouldSendRemove)
    {
        QRemoveObjectPacket p(m_name);
        io->write(p.serialize());
    }
    return listeners.length();
}

int QRemoteObjectSourcePrivate::qt_metacall(QMetaObject::Call call, int methodId, void **a)
{
    //We get called from the stored metaobject metacall.  Thus our index won't just be the index within our type, it will include
    //an offset for any signals/slots in the meta base class.  The delta offset accounts for this.
    const int delta = m_meta->methodCount() - m_methodOffset;

    methodId = QObject::qt_metacall(call, methodId, a);
    if (methodId < 0)
        return methodId;

    if (call == QMetaObject::InvokeMetaMethod) {
        if (methodId >= delta) {
            handleMetaCall(senderSignalIndex(), call, a);
        }
        --methodId;
    }

    return methodId;
}

QT_END_NAMESPACE
