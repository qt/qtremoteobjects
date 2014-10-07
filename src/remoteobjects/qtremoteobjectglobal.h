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

#ifndef QTREMOTEOBJECTGLOBAL_H
#define QTREMOTEOBJECTGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

typedef QPair<QString, QUrl> QRemoteObjectSourceLocation;
typedef QHash<QString, QUrl> QRemoteObjectSourceLocations;

Q_DECLARE_METATYPE(QRemoteObjectSourceLocation)
Q_DECLARE_METATYPE(QRemoteObjectSourceLocations)

#ifndef QT_STATIC
#  if defined(QT_BUILD_REMOTEOBJECTS_LIB)
#    define Q_REMOTEOBJECTS_EXPORT Q_DECL_EXPORT
#  else
#    define Q_REMOTEOBJECTS_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_REMOTEOBJECTS_EXPORT
#endif

#define QCLASSINFO_REMOTEOBJECT_TYPE "RemoteObject Type"

class QDataStream;
class QMetaObjectBuilder;

namespace QRemoteObjectStringLiterals {

// when QStringLiteral is used with the same string in different functions,
// it creates duplicate static data. Wrapping it in inline functions prevents it.

inline QString local() { return QStringLiteral("local"); }
inline QString tcp() { return QStringLiteral("tcp"); }

}

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
        PropertyChangePacket,
        ObjectList
    };

    QRemoteObjectPacket() { id = AddObject; }
    virtual ~QRemoteObjectPacket();
    virtual QByteArray serialize() const = 0;
    virtual bool deserialize(QDataStream&) = 0;
    static QRemoteObjectPacket* fromDataStream(QDataStream&);
    quint16 id;
};

class QInitPacketEncoder : public QRemoteObjectPacket
{
public:
    inline QInitPacketEncoder(const QString &_name, const QObject *_object, QMetaObject const *_base = Q_NULLPTR) :
        name(_name), object(_object), base(_base) { id = InitPacket; }
    QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    const QString name;
    const QObject *object;
    const QMetaObject *base;
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
    QInitDynamicPacketEncoder(const QString &_name, const QObject *_object, QMetaObject const *_base = Q_NULLPTR) :
        name(_name), object(_object), base(_base) { id = InitDynamicPacket; }
    QByteArray serialize() const Q_DECL_OVERRIDE;
    bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    const QString name;
    const QObject *object;
    const QMetaObject *base;
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
                                  QVector<int> &methodTypes,
                                  QVector<QVector<int> > &methodArgumentTypes,
                                  QVector<QPair<QByteArray, QVariant> > *propertyValues = 0) const;
    QString name;
    QByteArray packetData;
};

class QAddObjectPacket : public QRemoteObjectPacket
{
public:
    inline QAddObjectPacket() : QRemoteObjectPacket(),
       isDynamic(false) {}
    inline QAddObjectPacket(const QString &_name, bool _isDynamic) :
        name(_name), isDynamic(_isDynamic) { id = AddObject; }
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
    bool isDynamic;
};

class QRemoveObjectPacket : public QRemoteObjectPacket
{
public:
    inline QRemoveObjectPacket() : QRemoteObjectPacket() {}
    inline QRemoveObjectPacket(const QString &_name) :
       name(_name) { id = RemoveObject; }
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
};

class QInvokePacket : public QRemoteObjectPacket
{
public:
    inline QInvokePacket() : QRemoteObjectPacket(),
      call(-1), index(-1) {}
    inline QInvokePacket(const QString &_name, int _call, int _index, QVariantList _args) :
       name(_name), call(_call), index(_index), args(_args) { id = InvokePacket; }
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QString name;
    int call;
    int index;
    QVariantList args;
};

class QPropertyChangePacket : public QRemoteObjectPacket
{
public:
    inline QPropertyChangePacket() : QRemoteObjectPacket() {}
    inline QPropertyChangePacket(const QString &_name, const char *_propertyNameChar, const QVariant &_value) :
       name(_name), propertyName(_propertyNameChar), value(_value) { id = PropertyChangePacket; }
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
    inline QObjectListPacket(const QStringList &_objects) :
       objects(_objects) { id = ObjectList; }
    virtual QByteArray serialize() const Q_DECL_OVERRIDE;
    virtual bool deserialize(QDataStream&) Q_DECL_OVERRIDE;
    QStringList objects;
};

} // namespace QRemoteObjectPackets

namespace QtRemoteObjects {

Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(const QObject *src, QObject *dst);
Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(const QObject *src, QDataStream &dst);
Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(QDataStream &src, QObject *dst);

}

Q_DECLARE_LOGGING_CATEGORY(QT_REMOTEOBJECT)

QT_END_NAMESPACE

#endif // QTREMOTEOBJECTSGLOBAL_H
