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

#include "qremoteobjectpacket_p.h"

#include "qremoteobjectpendingcall.h"
#include "qremoteobjectsource.h"

#include "private/qmetaobjectbuilder_p.h"
#include "private/qremoteobjectsource_p.h"

QT_BEGIN_NAMESPACE

namespace QRemoteObjectPackets {

void serializeInitPacket(DataStreamPacket &ds, const QRemoteObjectSource *object)
{
    const QMetaObject *meta = object->m_object->metaObject();
    const QMetaObject *adapterMeta = Q_NULLPTR;
    if (object->hasAdapter())
        adapterMeta = object->m_adapter->metaObject();
    const SourceApiMap *api = object->m_api;

    ds.setId(InitPacket);
    ds << api->name();

    //Now copy the property data
    const int numProperties = api->propertyCount();
    ds << quint32(numProperties);  //Number of properties

    for (int i = 0; i < numProperties; ++i) {
        const int index = api->sourcePropertyIndex(i);
        if (index < 0) {
            qCWarning(QT_REMOTEOBJECT) << "QInitPacketEncoder - Found invalid property.  Index not found:" << i << "Dropping invalid packet.";
            ds.size = 0;
            return;
        }
        if (api->isAdapterProperty(i)) {
            const QMetaProperty mp = adapterMeta->property(index);
            ds << mp.read(object->m_adapter);
        } else {
            const QMetaProperty mp = meta->property(index);
            ds << mp.read(object->m_object);
        }
    }
    ds.finishPacket();
}

bool deSerializeQVariantList(QDataStream& s, QList<QVariant>& l)
{
    quint32 c;
    s >> c;
    const int initialListSize = l.size();
    if (static_cast<quint32>(l.size()) < c)
        l.reserve(c);
    else if (static_cast<quint32>(l.size()) > c)
        for (int i = c; i < initialListSize; ++i)
            l.removeLast();

    for (int i = 0; i < l.size(); ++i)
    {
        if (s.atEnd())
            return false;
        QVariant t;
        s >> t;
        l[i] = t;
    }
    for (quint32 i = l.size(); i < c; ++i)
    {
        if (s.atEnd())
            return false;
        QVariant t;
        s >> t;
        l.append(t);
    }
    return true;
}

void deserializeInitPacket(QDataStream &in, QVariantList &values)
{
    const bool success = deSerializeQVariantList(in, values);
    Q_ASSERT(success);
    Q_UNUSED(success);
}

void serializeInitDynamicPacket(DataStreamPacket &ds, const QRemoteObjectSource *object)
{
    const QMetaObject *meta = object->m_object->metaObject();
    const QMetaObject *adapterMeta = Q_NULLPTR;
    if (object->hasAdapter())
        adapterMeta = object->m_adapter->metaObject();
    const SourceApiMap *api = object->m_api;

    ds.setId(InitDynamicPacket);
    ds << api->name();

    //Now copy the property data
    const int numSignals = api->signalCount();
    ds << quint32(numSignals);  //Number of signals
    const int numMethods = api->methodCount();
    ds << quint32(numMethods);  //Number of methods

    for (int i = 0; i < numSignals; ++i) {
        const int index = api->sourceSignalIndex(i);
        if (index < 0) {
            qCWarning(QT_REMOTEOBJECT) << "QInitDynamicPacketEncoder - Found invalid signal.  Index not found:" << i << "Dropping invalid packet.";
            ds.size = 0;
            return;
        }
        ds << api->signalSignature(i);
    }

    for (int i = 0; i < numMethods; ++i) {
        const int index = api->sourceMethodIndex(i);
        if (index < 0) {
            qCWarning(QT_REMOTEOBJECT) << "QInitDynamicPacketEncoder - Found invalid method.  Index not found:" << i << "Dropping invalid packet.";
            ds.size = 0;
            return;
        }
        ds << api->methodSignature(i);
        ds << api->typeName(i);
    }

    const int numProperties = api->propertyCount();
    ds << quint32(numProperties);  //Number of properties

    for (int i = 0; i < numProperties; ++i) {
        const int index = api->sourcePropertyIndex(i);
        if (index < 0) {
            qCWarning(QT_REMOTEOBJECT) << "QInitDynamicPacketEncoder - Found invalid method.  Index not found:" << i << "Dropping invalid packet.";
            ds.size = 0;
            return;
        }
        if (api->isAdapterProperty(i)) {
            const QMetaProperty mp = adapterMeta->property(index);
            ds << mp.name();
            ds << mp.typeName();
            if (mp.notifySignalIndex() == -1)
                ds << QByteArray();
            else
                ds << mp.notifySignal().methodSignature();
            ds << mp.read(object->m_adapter);
        } else {
            const QMetaProperty mp = meta->property(index);
            ds << mp.name();
            ds << mp.typeName();
            if (mp.notifySignalIndex() == -1)
                ds << QByteArray();
            else
                ds << mp.notifySignal().methodSignature();
            ds << mp.read(object->m_object);
        }
    }
    ds.finishPacket();
}

void deserializeInitDynamicPacket(QDataStream &in, QMetaObjectBuilder &builder, QVariantList &values)
{
    quint32 numSignals = 0;
    quint32 numMethods = 0;
    quint32 numProperties = 0;

    in >> numSignals;
    in >> numMethods;

    int curIndex = 0;

    for (quint32 i = 0; i < numSignals; ++i) {
        QByteArray signature;
        in >> signature;
        ++curIndex;
        builder.addSignal(signature);
    }

    for (quint32 i = 0; i < numMethods; ++i) {
        QByteArray signature, returnType;

        in >> signature;
        in >> returnType;
        ++curIndex;
        const bool isVoid = returnType.isEmpty() || returnType == QByteArrayLiteral("void");
        if (isVoid)
            builder.addMethod(signature);
        else
            builder.addMethod(signature, QByteArrayLiteral("QRemoteObjectPendingCall"));
    }

    in >> numProperties;
    const quint32 initialListSize = values.size();
    if (static_cast<quint32>(values.size()) < numProperties)
        values.reserve(numProperties);
    else if (static_cast<quint32>(values.size()) > numProperties)
        for (quint32 i = numProperties; i < initialListSize; ++i)
            values.removeLast();

    for (quint32 i = 0; i < numProperties; ++i) {
        QByteArray name;
        QByteArray typeName;
        QByteArray signalName;
        in >> name;
        in >> typeName;
        in >> signalName;
        if (signalName.isEmpty())
            builder.addProperty(name, typeName);
        else
            builder.addProperty(name, typeName, builder.indexOfSignal(signalName));
        QVariant value;
        in >> value;
        if (i < initialListSize)
            values[i] = value;
        else
            values.append(value);
    }
}

void serializeAddObjectPacket(DataStreamPacket &ds, const QString &name, bool isDynamic)
{
    ds.setId(AddObject);
    ds << name;
    ds << isDynamic;
    ds.finishPacket();
}

void deserializeAddObjectPacket(QDataStream &ds, bool &isDynamic)
{
    ds >> isDynamic;
}

void serializeRemoveObjectPacket(DataStreamPacket &ds, const QString &name)
{
    ds.setId(RemoveObject);
    ds << name;
    ds.finishPacket();
}
//There is no deserializeRemoveObjectPacket - no parameters other than id and name

void serializeInvokePacket(DataStreamPacket &ds, const QString &name, int call, int index, const QVariantList *args, int serialId)
{
    ds.setId(InvokePacket);
    ds << name;
    ds << call;
    ds << index;
    ds << *args;
    ds << serialId;
    ds.finishPacket();
}

void deserializeInvokePacket(QDataStream& in, int &call, int &index, QVariantList &args, int &serialId)
{
    in >> call;
    in >> index;
    const bool success = deSerializeQVariantList(in, args);
    Q_ASSERT(success);
    Q_UNUSED(success);
    in >> serialId;
}

void serializeInvokeReplyPacket(DataStreamPacket &ds, const QString &name, int ackedSerialId, const QVariant &value)
{
    ds.setId(InvokeReplyPacket);
    ds << name;
    ds << ackedSerialId;
    ds << value;
    ds.finishPacket();
}

void deserializeInvokeReplyPacket(QDataStream& in, int &ackedSerialId, QVariant &value){
    in >> ackedSerialId;
    in >> value;
}

void serializePropertyChangePacket(DataStreamPacket &ds, const QString &name, const char *propertyName, const QVariant &value)
{
    ds.setId(PropertyChangePacket);
    ds << name;
    ds.writeBytes(propertyName, strlen(propertyName) + 1);
    ds << value;
    ds.finishPacket();
}

void deserializePropertyChangePacket(QDataStream& in, RawString &propertyName, QVariant &value)
{
    in >> propertyName;
    in >> value;
}

void serializeObjectListPacket(DataStreamPacket &ds, const QStringList &objects)
{
    ds.setId(ObjectList);
    ds << objects;
    ds.finishPacket();
}

void deserializeObjectListPacket(QDataStream &in, QStringList &objects)
{
    in >> objects;
}

} // namespace QRemoteObjectPackets

QT_END_NAMESPACE
