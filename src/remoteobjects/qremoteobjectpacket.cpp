/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qremoteobjectpacket_p.h"

#include "qremoteobjectpendingcall.h"
#include "qremoteobjectsource.h"
#include "qremoteobjectsource_p.h"

#include "private/qmetaobjectbuilder_p.h"

QT_BEGIN_NAMESPACE

using namespace QtRemoteObjects;

namespace QRemoteObjectPackets {

QVariant serializedProperty(const QMetaProperty &property, const QObject *object)
{
    const QVariant value = property.read(object);
    if (property.isEnumType()) {
        return QVariant::fromValue<qint32>(value.toInt());
    } else {
        return value; // return original
    }
}

QVariant deserializedProperty(const QVariant &in, const QMetaProperty &property)
{
    if (property.isEnumType()) {
        const qint32 enumValue = in.toInt();
        return QVariant(property.userType(), &enumValue);
    } else {
        return in; // return original
    }
}

void serializeHandshakePacket(DataStreamPacket &ds)
{
    ds.setId(Handshake);
    ds << QString(protocolVersion);
    ds.finishPacket();
}

void serializeInitPacket(DataStreamPacket &ds, const QRemoteObjectSource *object)
{
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

        const auto target = api->isAdapterProperty(i) ? object->m_adapter : object->m_object;
        const auto metaProperty = target->metaObject()->property(index);
        ds << serializedProperty(metaProperty, target);
    }
    ds.finishPacket();
}

bool deserializeQVariantList(QDataStream &s, QList<QVariant> &l)
{
    // note: optimized version of: QDataStream operator>>(QDataStream& s, QList<T>& l)
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
    const bool success = deserializeQVariantList(in, values);
    Q_ASSERT(success);
    Q_UNUSED(success);
}

void serializeInitDynamicPacket(DataStreamPacket &ds, const QRemoteObjectSource *object)
{
    const SourceApiMap *api = object->m_api;

    ds.setId(InitDynamicPacket);
    ds << api->name();

    //Now copy the property data
    const int numEnums = api->enumCount();
    const auto metaObject = object->m_object->metaObject();
    ds << quint32(numEnums);  //Number of Enums
    for (int i = 0; i < numEnums; ++i) {
        auto enumerator = metaObject->enumerator(api->sourceEnumIndex(i));
        Q_ASSERT(enumerator.isValid());
        ds << enumerator.name();
        ds << enumerator.isFlag();
        ds << enumerator.scope();
        const int keyCount = enumerator.keyCount();
        ds << keyCount;
        for (int k = 0; k < keyCount; ++k) {
            ds << enumerator.key(k);
            ds << enumerator.value(k);
        }
    }

    const int numSignals = api->signalCount();
    ds << quint32(numSignals);  //Number of signals
    for (int i = 0; i < numSignals; ++i) {
        const int index = api->sourceSignalIndex(i);
        if (index < 0) {
            qCWarning(QT_REMOTEOBJECT) << "QInitDynamicPacketEncoder - Found invalid signal.  Index not found:" << i << "Dropping invalid packet.";
            ds.size = 0;
            return;
        }
        ds << api->signalSignature(i);
        ds << api->signalParameterNames(i);
    }

    const int numMethods = api->methodCount();
    ds << quint32(numMethods);  //Number of methods
    for (int i = 0; i < numMethods; ++i) {
        const int index = api->sourceMethodIndex(i);
        if (index < 0) {
            qCWarning(QT_REMOTEOBJECT) << "QInitDynamicPacketEncoder - Found invalid method.  Index not found:" << i << "Dropping invalid packet.";
            ds.size = 0;
            return;
        }
        ds << api->methodSignature(i);
        ds << api->typeName(i);
        ds << api->methodParameterNames(i);
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

        const auto target = api->isAdapterProperty(i) ? object->m_adapter : object->m_object;
        const auto metaProperty = target->metaObject()->property(index);
        ds << metaProperty.name();
        ds << metaProperty.typeName();
        if (metaProperty.notifySignalIndex() == -1)
            ds << QByteArray();
        else
            ds << metaProperty.notifySignal().methodSignature();
        ds << metaProperty.read(target);
    }
    ds.finishPacket();
}

void deserializeInitDynamicPacket(QDataStream &in, QMetaObjectBuilder &builder, QVariantList &values)
{
    quint32 numEnums = 0;
    quint32 numSignals = 0;
    quint32 numMethods = 0;
    quint32 numProperties = 0;

    in >> numEnums;
    for (quint32 i = 0; i < numEnums; ++i) {
        QByteArray name;
        in >> name;
        auto enumBuilder = builder.addEnumerator(name);
        bool isFlag;
        in >> isFlag;
        enumBuilder.setIsFlag(isFlag);

        QByteArray scopeName;
        in >> scopeName; // scope
        // TODO uncomment this line after https://bugreports.qt.io/browse/QTBUG-64081 is implemented
        //enumBuilder.setScope(scopeName);

        int keyCount;
        in >> keyCount;
        for (int k = 0; k < keyCount; ++k) {
            QByteArray key;
            int value;
            in >> key;
            in >> value;
            enumBuilder.addKey(key, value);
        }
    }

    int curIndex = 0;

    in >> numSignals;
    for (quint32 i = 0; i < numSignals; ++i) {
        QByteArray signature;
        QList<QByteArray> paramNames;
        in >> signature;
        in >> paramNames;
        ++curIndex;
        auto mmb = builder.addSignal(signature);
        mmb.setParameterNames(paramNames);
    }

    in >> numMethods;
    for (quint32 i = 0; i < numMethods; ++i) {
        QByteArray signature, returnType;
        QList<QByteArray> paramNames;
        in >> signature;
        in >> returnType;
        in >> paramNames;
        ++curIndex;
        const bool isVoid = returnType.isEmpty() || returnType == QByteArrayLiteral("void");
        QMetaMethodBuilder mmb;
        if (isVoid)
            mmb = builder.addMethod(signature);
        else
            mmb = builder.addMethod(signature, QByteArrayLiteral("QRemoteObjectPendingCall"));
        mmb.setParameterNames(paramNames);
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

void serializeInvokePacket(DataStreamPacket &ds, const QString &name, int call, int index, const QVariantList &args, int serialId, int propertyIndex)
{
    ds.setId(InvokePacket);
    ds << name;
    ds << call;
    ds << index;

    ds << (quint32)args.size();
    foreach (const auto &arg, args) {
        if (QMetaType::typeFlags(arg.userType()).testFlag(QMetaType::IsEnumeration))
            ds << QVariant::fromValue<qint32>(arg.toInt());
        else
            ds << arg;
    }

    ds << serialId;
    ds << propertyIndex;
    ds.finishPacket();
}

void deserializeInvokePacket(QDataStream& in, int &call, int &index, QVariantList &args, int &serialId, int &propertyIndex)
{
    in >> call;
    in >> index;
    const bool success = deserializeQVariantList(in, args);
    Q_ASSERT(success);
    Q_UNUSED(success);
    in >> serialId;
    in >> propertyIndex;
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

void serializePropertyChangePacket(DataStreamPacket &ds, const QString &name, int index, const QVariant &value)
{
    ds.setId(PropertyChangePacket);
    ds << name;
    ds << index;
    ds << value;
    ds.finishPacket();
}

void deserializePropertyChangePacket(QDataStream& in, int &index, QVariant &value)
{
    in >> index;
    in >> value;
}

void serializeObjectListPacket(DataStreamPacket &ds, const ObjectInfoList &objects)
{
    ds.setId(ObjectList);
    ds << objects;
    ds.finishPacket();
}

void deserializeObjectListPacket(QDataStream &in, ObjectInfoList &objects)
{
    in >> objects;
}

} // namespace QRemoteObjectPackets

QT_END_NAMESPACE
