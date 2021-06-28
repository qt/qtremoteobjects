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

#include <QtCore/qabstractitemmodel.h>

#include "qremoteobjectpendingcall.h"
#include "qremoteobjectsource.h"
#include "qremoteobjectsource_p.h"
#include "qremoteobjectpacket_p.h"
#include "qconnectionfactories.h"
#include "qconnectionfactories_p.h"
#include <cstring>

//#define QTRO_VERBOSE_PROTOCOL
QT_BEGIN_NAMESPACE


// Add methods so we can use QMetaEnum in a set
// Note for both functions we are skipping string comparisons/hashes.  Since the
// metaObjects are the same, we can just use the address of the string.
inline bool operator==(const QMetaEnum e1, const QMetaEnum e2)
{
    return e1.enclosingMetaObject() == e2.enclosingMetaObject()
           && e1.name() == e2.name()
           && e1.enumName() == e2.enumName()
           && e1.scope() == e2.scope();
}

inline size_t qHash(const QMetaEnum &key, size_t seed=0) Q_DECL_NOTHROW
{
    return qHash(key.enclosingMetaObject(), seed) ^ qHash(static_cast<const void *>(key.name()), seed)
           ^ qHash(static_cast<const void *>(key.enumName()), seed) ^ qHash(static_cast<const void *>(key.scope()), seed);
}

using namespace QtRemoteObjects;

namespace QRemoteObjectPackets {

// QDataStream sends QVariants of custom types by sending their typename, allowing decode
// on the receiving side.  For QtRO and enums, this won't work, as the enums have different
// scopes.  E.g., the examples have ParentClassSource::MyEnum and ParentClassReplica::MyEnum.
// Dynamic types will be created as ParentClass::MyEnum.  So instead, we change the variants
// to integers (encodeVariant) when sending them.  On the receive side, the we know the
// types of properties and the signatures for methods, so we can use that information to
// decode the integer variant into an enum variant (via decodeVariant).
const QVariant encodeVariant(const QVariant &value)
{
    const auto metaType = value.metaType();
    if (metaType.flags().testFlag(QMetaType::IsEnumeration)) {
        auto converted = QVariant(value);
        const auto size = metaType.sizeOf();
        switch (size) {
        case 1: converted.convert(QMetaType::fromType<qint8>()); break;
        case 2: converted.convert(QMetaType::fromType<qint16>()); break;
        case 4: converted.convert(QMetaType::fromType<qint32>()); break;
        // Qt currently only supports enum values of 4 or less bytes (QMetaEnum value(index) returns int)
//        case 8: converted.convert(QMetaType::Long); break; // typeId for long from qmetatype.h
        default:
            qWarning() << "Invalid enum detected" << metaType.name() << "with size" << size;
            converted.convert(QMetaType::fromType<qint32>());
        }
#ifdef QTRO_VERBOSE_PROTOCOL
        qDebug() << "Converting from enum to integer type" << size << converted << value;
#endif
        return converted;
    }
    return value;
}

QVariant &decodeVariant(QVariant &value, QMetaType metaType)
{
    if (metaType.flags().testFlag(QMetaType::IsEnumeration)) {
#ifdef QTRO_VERBOSE_PROTOCOL
        QVariant encoded(value);
#endif
        value.convert(metaType);
#ifdef QTRO_VERBOSE_PROTOCOL
        qDebug() << "Converting to enum from integer type" << value << encoded;
#endif
    }
    return value;
}

void QDataStreamCodec::serializeProperty(const QRemoteObjectSourceBase *source, int internalIndex)
{
    serializeProperty(m_packet, source, internalIndex);
}

void QDataStreamCodec::serializeProperty(QDataStream &ds, const QRemoteObjectSourceBase *source, int internalIndex)
{
    const int propertyIndex = source->m_api->sourcePropertyIndex(internalIndex);
    Q_ASSERT (propertyIndex >= 0);
    const auto target = source->m_api->isAdapterProperty(internalIndex) ? source->m_adapter : source->m_object;
    const auto property = target->metaObject()->property(propertyIndex);
    const QVariant value = property.read(target);
    if (property.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
        auto const childSource = source->m_children.value(internalIndex);
        auto valueAsPointerToQObject = qvariant_cast<QObject *>(value);
        if (childSource->m_object != valueAsPointerToQObject)
            childSource->resetObject(valueAsPointerToQObject);
        QRO_ qro(childSource);
        if (source->d->isDynamic && qro.type == ObjectType::CLASS && childSource->m_object && !source->d->sentTypes.contains(qro.typeName)) {
            QDataStream classDef(&qro.classDefinition, QIODevice::WriteOnly);
            serializeDefinition(classDef, childSource);
            source->d->sentTypes.insert(qro.typeName);
        }
        ds << QVariant::fromValue<QRO_>(qro);
        if (qro.isNull)
            return;
        const int propertyCount = childSource->m_api->propertyCount();
        // Put the properties in a buffer, the receiver may not know how to
        // interpret the types until it registers new ones.
        QDataStream params(&qro.parameters, QIODevice::WriteOnly);
        params << propertyCount;
        for (int internalIndex = 0; internalIndex < propertyCount; ++internalIndex)
            serializeProperty(params, childSource, internalIndex);
        ds << qro.parameters;
        return;
    }
    if (source->d->isDynamic && property.userType() == QMetaType::QVariant
        && value.metaType().flags().testFlag(QMetaType::IsGadget)) {
        const auto typeName = QString::fromLatin1(value.metaType().name());
        if (!source->d->sentTypes.contains(typeName)) {
            QRO_ qro(value);
            ds << QVariant::fromValue<QRO_>(qro);
            ds << qro.parameters;
            source->d->sentTypes.insert(typeName);
            return;
        }
    }
    ds << encodeVariant(value);
}

void QDataStreamCodec::serializeHandshakePacket()
{
    m_packet.setId(Handshake);
    m_packet << QString(protocolVersion);
    m_packet.finishPacket();
}

void QDataStreamCodec::serializeInitPacket(const QRemoteObjectRootSource *source)
{
    m_packet.setId(InitPacket);
    m_packet << source->name();
    serializeProperties(source);
    m_packet.finishPacket();
}

void QDataStreamCodec::serializeProperties(const QRemoteObjectSourceBase *source)
{
    const SourceApiMap *api = source->m_api;

    //Now copy the property data
    const int numProperties = api->propertyCount();
    m_packet << quint32(numProperties);  //Number of properties

    for (int internalIndex = 0; internalIndex < numProperties; ++internalIndex)
        serializeProperty(source, internalIndex);
}

bool deserializeQVariantList(QDataStream &s, QVariantList &l)
{
    // note: optimized version of: QDataStream operator>>(QDataStream& s, QList<T>& l)
    quint32 c;
    s >> c;

    Q_ASSERT(static_cast<quint32>(l.size()) == l.size());
    if (static_cast<quint32>(l.size()) < c)
        l.reserve(c);
    else if (static_cast<quint32>(l.size()) > c)
        l.resize(c);

    for (int i = 0; i < l.size(); ++i)
    {
        if (s.atEnd())
            return false;
        s >> l[i];
    }
    for (auto i = l.size(); i < c; ++i)
    {
        if (s.atEnd())
            return false;
        s >> l.emplace_back();
    }
    return true;
}

void QDataStreamCodec::deserializeInitPacket(QDataStream &in, QVariantList &values)
{
    const bool success = deserializeQVariantList(in, values);
    Q_ASSERT(success);
    Q_UNUSED(success)
}

void QDataStreamCodec::serializeInitDynamicPacket(const QRemoteObjectRootSource *source)
{
    m_packet.setId(InitDynamicPacket);
    m_packet << source->name();
    serializeDefinition(m_packet, source);
    serializeProperties(source);
    m_packet.finishPacket();
}

static ObjectType getObjectType(const QString &typeName)
{
    if (typeName == QLatin1String("QAbstractItemModelAdapter"))
        return ObjectType::MODEL;
    auto tid = QMetaType::fromName(typeName.toUtf8()).id();
    if (tid == QMetaType::UnknownType)
        return ObjectType::CLASS;
    QMetaType type(tid);
    auto mo = type.metaObject();
    if (mo && mo->inherits(&QAbstractItemModel::staticMetaObject))
        return ObjectType::MODEL;
    return ObjectType::CLASS;
}

// Same method as in QVariant.cpp, as it isn't publicly exposed...
static QMetaEnum metaEnumFromType(QMetaType t)
{
    if (t.flags() & QMetaType::IsEnumeration) {
        if (const QMetaObject *metaObject = t.metaObject()) {
            const char *enumName = t.name();
            const char *lastColon = std::strrchr(enumName, ':');
            if (lastColon)
                enumName = lastColon + 1;
            return metaObject->enumerator(metaObject->indexOfEnumerator(enumName));
        }
    }
    return QMetaEnum();
}

static bool checkEnum(QMetaType metaType, QSet<QMetaEnum> &enums)
{
    if (metaType.flags().testFlag(QMetaType::IsEnumeration)) {
        QMetaEnum meta = metaEnumFromType(metaType);
        enums.insert(meta);
        return true;
    }
    return false;
}

static void recurseMetaobject(const QMetaObject *mo, QSet<const QMetaObject *> &gadgets, QSet<QMetaEnum> &enums)
{
    if (!mo || gadgets.contains(mo))
        return;
    gadgets.insert(mo);
    const int numProperties = mo->propertyCount();
    for (int i = 0; i < numProperties; ++i) {
        const auto property = mo->property(i);
        if (checkEnum(property.metaType(), enums))
            continue;
        if (property.metaType().flags().testFlag(QMetaType::IsGadget))
            recurseMetaobject(property.metaType().metaObject(), gadgets, enums);
    }
}

// A Source may only use a subset of the metaobjects properties/signals/slots, so we only search
// the ones in the API.  For nested pointer types, we will have another api to limit the search.
// For nested PODs/enums, we search the entire qobject (using the recurseMetaobject call()).
void recurseForGadgets(QSet<const QMetaObject *> &gadgets, QSet<QMetaEnum> &enums, const QRemoteObjectSourceBase *source)
{
    const SourceApiMap *api = source->m_api;

    const int numSignals = api->signalCount();
    const int numMethods = api->methodCount();
    const int numProperties = api->propertyCount();

    for (int si = 0; si < numSignals; ++si) {
        const int params = api->signalParameterCount(si);
        for (int pi = 0; pi < params; ++pi) {
            const int type = api->signalParameterType(si, pi);
            const auto metaType = QMetaType(type);
            if (checkEnum(metaType, enums))
                continue;
            if (!metaType.flags().testFlag(QMetaType::IsGadget))
                continue;
            const auto mo = metaType.metaObject();
            if (source->d->sentTypes.contains(QLatin1String(mo->className())))
                continue;
            recurseMetaobject(mo, gadgets, enums);
            source->d->sentTypes.insert(QLatin1String(mo->className()));
        }
    }

    for (int mi = 0; mi < numMethods; ++mi) {
        const int params = api->methodParameterCount(mi);
        for (int pi = 0; pi < params; ++pi) {
            const int type = api->methodParameterType(mi, pi);
            const auto metaType = QMetaType(type);
            if (checkEnum(metaType, enums))
                continue;
            if (!metaType.flags().testFlag(QMetaType::IsGadget))
                continue;
            const auto mo = metaType.metaObject();
            if (source->d->sentTypes.contains(QLatin1String(mo->className())))
                continue;
            recurseMetaobject(mo, gadgets, enums);
            source->d->sentTypes.insert(QLatin1String(mo->className()));
        }
    }
    for (int pi = 0; pi < numProperties; ++pi) {
        const int index = api->sourcePropertyIndex(pi);
        Q_ASSERT(index >= 0);
        const auto target = api->isAdapterProperty(pi) ? source->m_adapter : source->m_object;
        const auto metaProperty = target->metaObject()->property(index);
        const auto metaType = metaProperty.metaType();
        if (checkEnum(metaType, enums))
            continue;
        if (metaType.flags().testFlag(QMetaType::PointerToQObject)) {
            auto const objectType = getObjectType(QString::fromLatin1(metaProperty.typeName()));
            if (objectType == ObjectType::CLASS) {
                auto const childSource = source->m_children.value(pi);
                if (childSource->m_object)
                    recurseForGadgets(gadgets, enums, childSource);
            }
        }
        if (!metaType.flags().testFlag(QMetaType::IsGadget))
            continue;
        const auto mo = metaType.metaObject();
        if (source->d->sentTypes.contains(QLatin1String(mo->className())))
            continue;
        recurseMetaobject(mo, gadgets, enums);
        source->d->sentTypes.insert(QLatin1String(mo->className()));
    }
}

static bool checkForEnumsInSource(const QMetaObject *meta, const QRemoteObjectSourceBase *source)
{
    if (source->m_object->inherits(meta->className()))
        return true;
    for (const auto &child : source->m_children) {
        if (child->m_object && checkForEnumsInSource(meta, child))
            return true;
    }
    return false;
}

static void serializeEnum(QDataStream &ds, const QMetaEnum &enumerator)
{
    ds << QByteArray::fromRawData(enumerator.name(), qsizetype(qstrlen(enumerator.name())));
    ds << enumerator.isFlag();
    ds << enumerator.isScoped();
    const auto typeName = QByteArray(enumerator.scope()).append("::").append(enumerator.name());
    const quint32 size = quint32(QMetaType::fromName(typeName.constData()).sizeOf());
    ds << size;
#ifdef QTRO_VERBOSE_PROTOCOL
    qDebug("  Enum (name = %s, size = %d, isFlag = %s, isScoped = %s):", enumerator.name(), size, enumerator.isFlag() ? "true" : "false", enumerator.isScoped() ? "true" : "false");
#endif
    const int keyCount = enumerator.keyCount();
    ds << keyCount;
    for (int k = 0; k < keyCount; ++k) {
        ds << QByteArray::fromRawData(enumerator.key(k), qsizetype(qstrlen(enumerator.key(k))));
        ds << enumerator.value(k);
#ifdef QTRO_VERBOSE_PROTOCOL
        qDebug("    Key %d (name = %s, value = %d):", k, enumerator.key(k), enumerator.value(k));
#endif
    }
}

static void serializeGadgets(QDataStream &ds, const QSet<const QMetaObject *> &gadgets, const QSet<QMetaEnum> &enums, const QRemoteObjectSourceBase *source=nullptr)
{
    // Determine how to handle the enums found
    QSet<QMetaEnum> qtEnums;
    QSet<const QMetaObject *> dynamicEnumMetaObjects;
    for (const auto &metaEnum : enums) {
        auto const metaObject = metaEnum.enclosingMetaObject();
        if (gadgets.contains(metaObject)) // Part of a gadget will we serialize
            continue;
        // This checks if the enum is defined in our object heirarchy, in which case it will
        // already have been serialized.
        if (source && checkForEnumsInSource(metaObject, source->d->root))
            continue;
        // qtEnums are enumerations already known by Qt, so we only need register them.
        // We don't need to send all of the key/value data.
        if (metaObject == &Qt::staticMetaObject) // Are the other Qt metaclasses for enums?
            qtEnums.insert(metaEnum);
        else
            dynamicEnumMetaObjects.insert(metaEnum.enclosingMetaObject());
    }
    ds << quint32(qtEnums.size());
    for (const auto &metaEnum : qtEnums) {
        QByteArray enumName(metaEnum.scope());
        enumName.append("::", 2).append(metaEnum.name());
        ds << enumName;
    }
    const auto allMetaObjects = gadgets + dynamicEnumMetaObjects;
    ds << quint32(allMetaObjects.size());
#ifdef QTRO_VERBOSE_PROTOCOL
    qDebug() << "  Found" << gadgets.size() << "gadget/pod and" << (allMetaObjects.size() - gadgets.size()) << "enum types";
    int i = 0;
#endif
    // There isn't an easy way to update a metaobject incrementally, so we
    // send all of the metaobject's enums, but no properties, when an external
    // enum is requested.
    for (auto const meta : allMetaObjects) {
        ds << QByteArray::fromRawData(meta->className(), qsizetype(qstrlen(meta->className())));
        int propertyCount = gadgets.contains(meta) ? meta->propertyCount() : 0;
        ds << quint32(propertyCount);
#ifdef QTRO_VERBOSE_PROTOCOL
        qDebug("  Gadget %d (name = %s, # properties = %d, # enums = %d):", i++, meta->className(), propertyCount, meta->enumeratorCount());
#endif
        for (int j = 0; j < propertyCount; j++) {
            auto prop = meta->property(j);
#ifdef QTRO_VERBOSE_PROTOCOL
            qDebug("    Data member %d (name = %s, type = %s):", j, prop.name(), prop.typeName());
#endif
            ds << QByteArray::fromRawData(prop.name(), qsizetype(qstrlen(prop.name())));
            ds << QByteArray::fromRawData(prop.typeName(), qsizetype(qstrlen(prop.typeName())));
        }
        int enumCount = meta->enumeratorCount();
        ds << quint32(enumCount);
        for (int j = 0; j < enumCount; j++) {
            auto const enumMeta = meta->enumerator(j);
            serializeEnum(ds, enumMeta);
        }
    }
}

void QDataStreamCodec::serializeDefinition(QDataStream &ds, const QRemoteObjectSourceBase *source)
{
    const SourceApiMap *api = source->m_api;
    const QByteArray desiredClassName(api->typeName().toLatin1());
    const QByteArray originalClassName = api->className();
    // The dynamic class will be called typeName on the receiving side of this definition
    // However, there are types like enums that have the QObject's class name.  Replace()
    // will convert a parameter such as "ParentClassSource::MyEnum" to "ParentClass::MyEnum"
    // so the type can be properly resolved and registered.
    auto replace = [&originalClassName, &desiredClassName](QByteArray &name) {
        name.replace(originalClassName, desiredClassName);
    };

    ds << source->m_api->typeName();
#ifdef QTRO_VERBOSE_PROTOCOL
    qDebug() << "Serializing definition for" << source->m_api->typeName();
#endif

    //Now copy the property data
    const int numEnums = api->enumCount();
    const auto metaObject = source->m_object->metaObject();
    ds << quint32(numEnums);  //Number of Enums
#ifdef QTRO_VERBOSE_PROTOCOL
    qDebug() << "  Found" << numEnums << "enumeration types";
#endif
    for (int i = 0; i < numEnums; ++i) {
        auto enumerator = metaObject->enumerator(api->sourceEnumIndex(i));
        Q_ASSERT(enumerator.isValid());
        serializeEnum(ds, enumerator);
    }

    if (source->d->isDynamic) {
        QSet<const QMetaObject *> gadgets;
        QSet<QMetaEnum> enums;
        recurseForGadgets(gadgets, enums, source);
        serializeGadgets(ds, gadgets, enums, source);
    } else
        ds << quint32(0) << quint32(0); // qtEnums, numGadgets

    const int numSignals = api->signalCount();
    ds << quint32(numSignals);  //Number of signals
    for (int i = 0; i < numSignals; ++i) {
        const int index = api->sourceSignalIndex(i);
        Q_ASSERT(index >= 0);
        auto signature = api->signalSignature(i);
        replace(signature);
#ifdef QTRO_VERBOSE_PROTOCOL
        qDebug() << "  Signal" << i << "(signature =" << signature << "parameter names =" << api->signalParameterNames(i) << ")";
#endif
        ds << signature;
        ds << api->signalParameterNames(i);
    }

    const int numMethods = api->methodCount();
    ds << quint32(numMethods);  //Number of methods
    for (int i = 0; i < numMethods; ++i) {
        const int index = api->sourceMethodIndex(i);
        Q_ASSERT(index >= 0);
        auto signature = api->methodSignature(i);
        replace(signature);
        auto typeName = api->typeName(i);
        replace(typeName);
#ifdef QTRO_VERBOSE_PROTOCOL
        qDebug() << "  Slot" << i << "(signature =" << signature << "parameter names =" << api->methodParameterNames(i) << "return type =" << typeName << ")";
#endif
        ds << signature;
        ds << typeName;
        ds << api->methodParameterNames(i);
    }

    const int numProperties = api->propertyCount();
    ds << quint32(numProperties);  //Number of properties
    for (int i = 0; i < numProperties; ++i) {
        const int index = api->sourcePropertyIndex(i);
        Q_ASSERT(index >= 0);

        const auto target = api->isAdapterProperty(i) ? source->m_adapter : source->m_object;
        const auto metaProperty = target->metaObject()->property(index);
        ds << metaProperty.name();
#ifdef QTRO_VERBOSE_PROTOCOL
        qDebug() << "  Property" << i << "name =" << metaProperty.name();
#endif
        if (metaProperty.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
            auto objectType = getObjectType(QLatin1String(metaProperty.typeName()));
            ds << (objectType == ObjectType::CLASS ? "QObject*" : "QAbstractItemModel*");
#ifdef QTRO_VERBOSE_PROTOCOL
            qDebug() << "    Type:" << (objectType == ObjectType::CLASS ? "QObject*" : "QAbstractItemModel*");
#endif
        } else {
            ds << metaProperty.typeName();
#ifdef QTRO_VERBOSE_PROTOCOL
            qDebug() << "    Type:" << metaProperty.typeName();
#endif
        }
        if (metaProperty.notifySignalIndex() == -1) {
            ds << QByteArray();
#ifdef QTRO_VERBOSE_PROTOCOL
            qDebug() << "    Notification signal: None";
#endif
        } else {
            auto signature = metaProperty.notifySignal().methodSignature();
            replace(signature);
            ds << signature;
#ifdef QTRO_VERBOSE_PROTOCOL
            qDebug() << "    Notification signal:" << signature;
#endif
        }
    }
}

void QDataStreamCodec::serializeAddObjectPacket(const QString &name, bool isDynamic)
{
    m_packet.setId(AddObject);
    m_packet << name;
    m_packet << isDynamic;
    m_packet.finishPacket();
}

void QDataStreamCodec::deserializeAddObjectPacket(QDataStream &ds, bool &isDynamic)
{
    ds >> isDynamic;
}

void QDataStreamCodec::serializeRemoveObjectPacket(const QString &name)
{
    m_packet.setId(RemoveObject);
    m_packet << name;
    m_packet.finishPacket();
}
//There is no deserializeRemoveObjectPacket - no parameters other than id and name

void QDataStreamCodec::serializeInvokePacket(const QString &name, int call, int index, const QVariantList &args, int serialId, int propertyIndex)
{
    m_packet.setId(InvokePacket);
    m_packet << name;
    m_packet << call;
    m_packet << index;

    m_packet << quint32(args.size());
    for (const auto &arg : args)
        m_packet << encodeVariant(arg);

    m_packet << serialId;
    m_packet << propertyIndex;
    m_packet.finishPacket();
}

void QDataStreamCodec::deserializeInvokePacket(QDataStream& in, int &call, int &index, QVariantList &args, int &serialId, int &propertyIndex)
{
    in >> call;
    in >> index;
    const bool success = deserializeQVariantList(in, args);
    Q_ASSERT(success);
    Q_UNUSED(success)
    in >> serialId;
    in >> propertyIndex;
}

void QDataStreamCodec::serializeInvokeReplyPacket(const QString &name, int ackedSerialId, const QVariant &value)
{
    m_packet.setId(InvokeReplyPacket);
    m_packet << name;
    m_packet << ackedSerialId;
    m_packet << value;
    m_packet.finishPacket();
}

void QDataStreamCodec::deserializeInvokeReplyPacket(QDataStream& in, int &ackedSerialId, QVariant &value){
    in >> ackedSerialId;
    in >> value;
}

void QDataStreamCodec::serializePropertyChangePacket(QRemoteObjectSourceBase *source, int signalIndex)
{
    int internalIndex = source->m_api->propertyRawIndexFromSignal(signalIndex);
    m_packet.setId(PropertyChangePacket);
    m_packet << source->name();
    m_packet << internalIndex;
    serializeProperty(source, internalIndex);
    m_packet.finishPacket();
}

void QDataStreamCodec::deserializePropertyChangePacket(QDataStream& in, int &index, QVariant &value)
{
    in >> index;
    in >> value;
}

void QDataStreamCodec::serializeObjectListPacket(const ObjectInfoList &objects)
{
    m_packet.setId(ObjectList);
    m_packet << objects;
    m_packet.finishPacket();
}

void QDataStreamCodec::deserializeObjectListPacket(QDataStream &in, ObjectInfoList &objects)
{
    in >> objects;
}

void QDataStreamCodec::serializePingPacket(const QString &name)
{
    m_packet.setId(Ping);
    m_packet << name;
    m_packet.finishPacket();
}

void QDataStreamCodec::serializePongPacket(const QString &name)
{
    m_packet.setId(Pong);
    m_packet << name;
    m_packet.finishPacket();
}

QRO_::QRO_(QRemoteObjectSourceBase *source)
    : name(source->name())
    , typeName(source->m_api->typeName())
    , type(source->m_adapter ? ObjectType::MODEL : getObjectType(typeName))
    , isNull(source->m_object == nullptr)
    , classDefinition()
    , parameters()
{}

QRO_::QRO_(const QVariant &value)
    : type(ObjectType::GADGET)
    , isNull(false)
{
    const auto metaType = value.metaType();
    auto meta = metaType.metaObject();
    QDataStream out(&classDefinition, QIODevice::WriteOnly);
    const int numProperties = meta->propertyCount();
    const auto name = metaType.name();
    const auto typeName = QByteArray::fromRawData(name, qsizetype(qstrlen(name)));
    out << quint32(0) << quint32(1);
    out << typeName;
    out << numProperties;
#ifdef QTRO_VERBOSE_PROTOCOL
    qDebug("Serializing POD definition to QRO_ (name = %s)", typeName.constData());
#endif
    for (int i = 0; i < numProperties; ++i) {
        const auto property = meta->property(i);
#ifdef QTRO_VERBOSE_PROTOCOL
        qDebug("  Data member %d (name = %s, type = %s):", i, property.name(), property.typeName());
#endif
        out << QByteArray::fromRawData(property.name(), qsizetype(qstrlen(property.name())));
        out << QByteArray::fromRawData(property.typeName(), qsizetype(qstrlen(property.typeName())));
    }
    out << quint32(0); // PODs have no enums
    QDataStream ds(&parameters, QIODevice::WriteOnly);
    ds << value;
#ifdef QTRO_VERBOSE_PROTOCOL
    qDebug() << "  Value:" << value;
#endif
}

QDataStream &operator<<(QDataStream &stream, const QRO_ &info)
{
    stream << info.name << info.typeName << quint8(info.type) << info.classDefinition << info.isNull;
    qCDebug(QT_REMOTEOBJECT) << "Serializing " << info;
    // info.parameters will be filled in by serializeProperty
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QRO_ &info)
{
    quint8 tmpType;
    stream >> info.name >> info.typeName >> tmpType >> info.classDefinition >> info.isNull;
    info.type = static_cast<ObjectType>(tmpType);
    qCDebug(QT_REMOTEOBJECT) << "Deserializing " << info;
    if (!info.isNull)
        stream >> info.parameters;
    return stream;
}

DataStreamPacket::DataStreamPacket(quint16 id)
    : QDataStream(&array, QIODevice::WriteOnly), baseAddress(0), size(0)
{
    this->setVersion(QtRemoteObjects::dataStreamVersion);
    *this << quint32(0);
    *this << id;
}

void CodecBase::send(const QSet<QtROIoDeviceBase *> &connections)
{
    const auto bytearray = getPayload();
    for (auto conn : connections)
        conn->write(bytearray);
    reset();
}

void CodecBase::send(const QList<QtROIoDeviceBase *> &connections)
{
    const auto bytearray = getPayload();
    for (auto conn : connections)
        conn->write(bytearray);
    reset();
}

void CodecBase::send(QtROIoDeviceBase *connection)
{
    const auto bytearray = getPayload();
    connection->write(bytearray);
    reset();
}

} // namespace QRemoteObjectPackets

QT_END_NAMESPACE
