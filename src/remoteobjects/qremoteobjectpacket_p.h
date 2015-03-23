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

QT_BEGIN_NAMESPACE

class QMetaObjectBuilder;
class QRemoteObjectSourcePrivate;

namespace QRemoteObjectPackets {

//Helper class for creating a QByteArray from a QRemoteObjectPacket
class DataStreamPacket : public QDataStream
{
public:
    DataStreamPacket(quint16 id) : QDataStream(&array, QIODevice::WriteOnly)
    {
        *this << quint32(0);
        *this << id;
    }
    QByteArray finishPacket()
    {
        this->device()->seek(0);
        *this << quint32(array.length() - sizeof(quint32));
        return array;
    }
    QByteArray array;

private:
    Q_DISABLE_COPY(DataStreamPacket)
};

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
    virtual QByteArray serialize() const = 0;
    virtual bool deserialize(QDataStream&) = 0;
    static QRemoteObjectPacket* fromDataStream(QDataStream&);
    quint16 id;
};

class QInitPacketEncoder : public QRemoteObjectPacket
{
public:
    inline QInitPacketEncoder(const QRemoteObjectSourcePrivate *_object)
        : QRemoteObjectPacket(InitPacket), object(_object) {}
    QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    const QRemoteObjectSourcePrivate *object;
private:
    QInitPacketEncoder() {}
};

class QInitPacket : public QRemoteObjectPacket
{
public:
    inline QInitPacket() : QRemoteObjectPacket() {}
    QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
    QByteArray packetData;
};

class QInitDynamicPacketEncoder : public QRemoteObjectPacket
{
public:
    QInitDynamicPacketEncoder(const QRemoteObjectSourcePrivate *_object)
        : QRemoteObjectPacket(InitDynamicPacket), object(_object) {}
    QByteArray serialize() const Q_DECL_OVERRIDE;
    bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    const QRemoteObjectSourcePrivate *object;
private:
    QInitDynamicPacketEncoder() {}
};

class QInitDynamicPacket : public QRemoteObjectPacket
{
public:
    inline QInitDynamicPacket() : QRemoteObjectPacket() {}
    QByteArray serialize() const Q_DECL_OVERRIDE;
    bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QMetaObject *createMetaObject(QMetaObjectBuilder &builder,
                                  int &outNumSignals,
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
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
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
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
};

class QInvokePacket : public QRemoteObjectPacket
{
public:
    inline QInvokePacket() : QRemoteObjectPacket(),
      call(-1), index(-1), serialId(-1) {}
    inline QInvokePacket(const QString &_name, int _call, int _index, QVariantList _args, int _serialId = -1)
        : QRemoteObjectPacket(InvokePacket), name(_name), call(_call), index(_index), args(_args), serialId(_serialId) {}
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
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
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;

    QString name;
    int ackedSerialId;

    // reply payload
    QVariant value;
};

class QPropertyChangePacket : public QRemoteObjectPacket
{
public:
    inline QPropertyChangePacket() : QRemoteObjectPacket() {}
    inline QPropertyChangePacket(const QString &_name, const char *_propertyNameChar, const QVariant &_value)
        : QRemoteObjectPacket(PropertyChangePacket), name(_name), propertyName(_propertyNameChar), value(_value) {}
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
    QByteArray propertyName;
    QVariant value;
};

class QObjectListPacket : public QRemoteObjectPacket
{
public:
    inline QObjectListPacket() : QRemoteObjectPacket() {}
    inline QObjectListPacket(const QStringList &_objects)
        : QRemoteObjectPacket(ObjectList), objects(_objects) {}
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QStringList objects;
};

} // namespace QRemoteObjectPackets

QT_END_NAMESPACE

#endif
