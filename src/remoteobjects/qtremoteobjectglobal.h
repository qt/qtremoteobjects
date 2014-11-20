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

namespace QtRemoteObjects {

Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(const QObject *src, QObject *dst);
Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(const QObject *src, QDataStream &dst);
Q_REMOTEOBJECTS_EXPORT void copyStoredProperties(QDataStream &src, QObject *dst);

}

Q_DECLARE_LOGGING_CATEGORY(QT_REMOTEOBJECT)

QT_END_NAMESPACE

#endif // QTREMOTEOBJECTSGLOBAL_H
