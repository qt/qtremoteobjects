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
#include <QtCore/QUrl>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

struct QRemoteObjectSourceLocationInfo
{
    QRemoteObjectSourceLocationInfo() {}
    QRemoteObjectSourceLocationInfo(const QString &typeName_, const QUrl &hostUrl_)
        : typeName(typeName_), hostUrl(hostUrl_) {}

    QRemoteObjectSourceLocationInfo &operator=(const QRemoteObjectSourceLocationInfo &other)
    {
        typeName = other.typeName;
        hostUrl = other.hostUrl;
        return *this;
    }

    inline bool operator==(const QRemoteObjectSourceLocationInfo &other) const Q_DECL_NOTHROW
    {
        return other.typeName == typeName && other.hostUrl == hostUrl;
    }
    inline bool operator!=(const QRemoteObjectSourceLocationInfo &other) const Q_DECL_NOTHROW
    {
        return !(*this == other);
    }

    QString typeName;
    QUrl hostUrl;
};

inline QDebug operator<<(QDebug dbg, const QRemoteObjectSourceLocationInfo &info)
{
    dbg.nospace() << "SourceLocationInfo(" << info.typeName << ", " << info.hostUrl << ")";
    return dbg.space();
}

inline QDataStream& operator<<(QDataStream &stream, const QRemoteObjectSourceLocationInfo &info)
{
    return stream << info.typeName << info.hostUrl;
}

inline QDataStream& operator>>(QDataStream &stream, QRemoteObjectSourceLocationInfo &info)
{
    return stream >> info.typeName >> info.hostUrl;
}

typedef QPair<QString, QRemoteObjectSourceLocationInfo> QRemoteObjectSourceLocation;
typedef QHash<QString, QRemoteObjectSourceLocationInfo> QRemoteObjectSourceLocations;
typedef QHash<int, QByteArray> QIntHash;

Q_DECLARE_METATYPE(QRemoteObjectSourceLocation)
Q_DECLARE_METATYPE(QRemoteObjectSourceLocations)
Q_DECLARE_METATYPE(QIntHash)

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

namespace QRemoteObjectStringLiterals {

// when QStringLiteral is used with the same string in different functions,
// it creates duplicate static data. Wrapping it in inline functions prevents it.

inline QString local() { return QStringLiteral("local"); }
inline QString tcp() { return QStringLiteral("tcp"); }

}

Q_DECLARE_LOGGING_CATEGORY(QT_REMOTEOBJECT)
Q_DECLARE_LOGGING_CATEGORY(QT_REMOTEOBJECT_MODELS)
Q_DECLARE_LOGGING_CATEGORY(QT_REMOTEOBJECT_IO)

namespace QtRemoteObjects {

Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(const QMetaObject *mo, const void *src, void *dst);
Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(const QMetaObject *mo, const void *src, QDataStream &dst);
Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(const QMetaObject *mo, QDataStream &src, void *dst);

template <typename T>
void copyStoredProperties(const T *src, T *dst)
{
    copyStoredProperties(&T::staticMetaObject, src, dst);
}

template <typename T>
void copyStoredProperties(const T *src, QDataStream &dst)
{
    copyStoredProperties(&T::staticMetaObject, src, dst);
}

template <typename T>
void copyStoredProperties(QDataStream &src, T *dst)
{
    copyStoredProperties(&T::staticMetaObject, src, dst);
}

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

}

QT_END_NAMESPACE

#endif // QTREMOTEOBJECTSGLOBAL_H
