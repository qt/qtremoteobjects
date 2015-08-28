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

#ifndef QTREMOTEOBJECTPACKET_P_H
#define QTREMOTEOBJECTPACKET_P_H

#include "qtremoteobjectglobal.h"
#include "qremoteobjectsource.h"

#include <QtCore/QDataStream>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QLoggingCategory>

#include <cstdlib>

QT_BEGIN_NAMESPACE

class QMetaObjectBuilder;
class QRemoteObjectSource;

namespace QRemoteObjectPackets {

const int dataStreamVersion = QDataStream::Qt_5_0;

class DataStreamPacket;

class QRemoteObjectPacket
{
public:
    enum QRemoteObjectPacketTypeEnum
    {
        Invalid = 0,
        InitPacket,
        InitDynamicPacket,
        AddObject,
        RemoveObject,
        InvokePacket,
        InvokeReplyPacket,
        PropertyChangePacket,
        ObjectList
    };

    QRemoteObjectPacket(quint64 _id = Invalid) : id(_id) {}
    virtual ~QRemoteObjectPacket();
    virtual void serialize(DataStreamPacket *) const = 0;
    virtual bool deserialize(QDataStream&) = 0;
    static QRemoteObjectPacket* fromDataStream(QDataStream&, QVector<QRemoteObjectPacket*> *buffer);
    quint16 id;
};

//Helper class for creating a QByteArray from a QRemoteObjectPacket
class DataStreamPacket : public QDataStream
{
public:
    DataStreamPacket(quint16 id = QRemoteObjectPacket::InvokePacket)
        : QDataStream(&array, QIODevice::WriteOnly)
        , baseAddress(0)
    {
        this->setVersion(dataStreamVersion);
        *this << quint32(0);
        *this << id;
    }
    void setId(quint16 id)
    {
        device()->seek(baseAddress);
        *this << quint32(0);
        *this << id;
    }

    void finishPacket()
    {
        size = device()->pos();
        device()->seek(baseAddress);
        *this << quint32(size - baseAddress - sizeof(quint32));
    }
    QByteArray array;
    int baseAddress;
    int size;

private:
    Q_DISABLE_COPY(DataStreamPacket)
};

class QInitPacketEncoder : public QRemoteObjectPacket
{
public:
    inline QInitPacketEncoder(const QRemoteObjectSource *_object)
        : QRemoteObjectPacket(InitPacket), object(_object) {}
    void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    const QRemoteObjectSource *object;
private:
    QInitPacketEncoder() {}
};

class QInitPacket : public QRemoteObjectPacket
{
public:
    inline QInitPacket() : QRemoteObjectPacket() {}
    void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
    QByteArray packetData;
};

class QInitDynamicPacketEncoder : public QRemoteObjectPacket
{
public:
    QInitDynamicPacketEncoder(const QRemoteObjectSource *_object)
        : QRemoteObjectPacket(InitDynamicPacket), object(_object) {}
    void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    const QRemoteObjectSource *object;
private:
    QInitDynamicPacketEncoder() {}
};

class QInitDynamicPacket : public QRemoteObjectPacket
{
public:
    inline QInitDynamicPacket() : QRemoteObjectPacket() {}
    void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QMetaObject *createMetaObject(QMetaObjectBuilder &builder,
                                  QVector<QPair<QByteArray, QVariant> > *propertyValues = 0) const;
    QString name;
    QByteArray packetData;
};

class QAddObjectPacket : public QRemoteObjectPacket
{
public:
    inline QAddObjectPacket() : QRemoteObjectPacket(),
       isDynamic(false) {}
    inline QAddObjectPacket(const QString &_name, bool _isDynamic)
        : QRemoteObjectPacket(AddObject), name(_name), isDynamic(_isDynamic) {}
    virtual void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
    bool isDynamic;
};

class QRemoveObjectPacket : public QRemoteObjectPacket
{
public:
    inline QRemoveObjectPacket() : QRemoteObjectPacket() {}
    inline QRemoveObjectPacket(const QString &_name)
        : QRemoteObjectPacket(RemoveObject), name(_name) {}
    virtual void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
};

void serializeInvokePacket(DataStreamPacket*, const QString &name, int call, int index, const QVariantList *args, int serialId = -1);

class QInvokePacket : public QRemoteObjectPacket
{
public:
    inline QInvokePacket() : QRemoteObjectPacket(),
      call(-1), index(-1), serialId(-1) {}
    inline QInvokePacket(const QString &_name, int _call, int _index, QVariantList _args, int _serialId = -1)
        : QRemoteObjectPacket(InvokePacket), name(_name), call(_call), index(_index), args(_args), serialId(_serialId) {}
    virtual void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;

    QString name;
    int call;
    int index;
    QVariantList args;

    int serialId;
};

class QInvokeReplyPacket : public QRemoteObjectPacket
{
public:
    inline QInvokeReplyPacket() : QRemoteObjectPacket()
        , ackedSerialId(-1) {}
    inline QInvokeReplyPacket(const QString &_name, int _ackedSerialId, const QVariant &_value = QVariant())
        : QRemoteObjectPacket(InvokeReplyPacket), name(_name), ackedSerialId(_ackedSerialId), value(_value) {}
    virtual void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;

    QString name;
    int ackedSerialId;

    // reply payload
    QVariant value;
};

//TODO do we need the object name or could we go with an id in backend code, this could be a costly allocation
void serializePropertyChangePacket(DataStreamPacket *stream, const QString &name, const char *propertyName, const QVariant &value);

struct RawString {
    RawString() : string(Q_NULLPTR), m_size(0u), bufferSize(0u){}
    RawString(const char *input)
        : string(Q_NULLPTR)
        , m_size(0u)
        , bufferSize(0u)
    {
        setString(input);
    }
    ~RawString()
    {
        free(string);
    }

    void setString(const char *data, int size = -1)
    {
        if (size == -1)
            m_size = strlen(data);
        else
            m_size = size;
        realloc(m_size);
        memcpy(string, data, m_size);
    }
private:
    void realloc(size_t size)
    {
        if (bufferSize < size) {
            string = static_cast<char*>(std::realloc(string, size));
            bufferSize = size;
            m_size = bufferSize;
        }
    }
    inline friend QDataStream &operator >>(QDataStream &stream, RawString &value);
public:
    char *string;
    size_t m_size;
    size_t bufferSize;
};

inline QDataStream &operator >>(QDataStream &stream, RawString &value)
{
    quint32 len;
    stream >> len;
    if (len == 0xffffffff)
        return stream;
    if (value.bufferSize < len)
        value.realloc(len);

    const int read = stream.readRawData(value.string, len);
    value.m_size = read;
    Q_ASSERT(value.m_size == len);

    return stream;
}

class QPropertyChangePacket : public QRemoteObjectPacket
{
public:
    inline QPropertyChangePacket() : QRemoteObjectPacket() {}
    inline QPropertyChangePacket(const QString &_name, const char *_propertyNameChar, const QVariant &_value)
        : QRemoteObjectPacket(PropertyChangePacket), name(_name), propertyName(_propertyNameChar), value(_value) {}
    virtual void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
    RawString propertyName;
    QVariant value;
};

class QObjectListPacket : public QRemoteObjectPacket
{
public:
    inline QObjectListPacket() : QRemoteObjectPacket() {}
    inline QObjectListPacket(const QStringList &_objects)
        : QRemoteObjectPacket(ObjectList), objects(_objects) {}
    virtual void serialize(DataStreamPacket*) const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QStringList objects;
};

} // namespace QRemoteObjectPackets

QT_END_NAMESPACE

#endif
