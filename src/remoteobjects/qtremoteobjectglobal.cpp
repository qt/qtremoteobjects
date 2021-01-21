/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qtremoteobjectglobal.h"

#include <QtCore/qdatastream.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_REMOTEOBJECT, "qt.remoteobjects", QtWarningMsg)
Q_LOGGING_CATEGORY(QT_REMOTEOBJECT_MODELS, "qt.remoteobjects.models", QtWarningMsg)
Q_LOGGING_CATEGORY(QT_REMOTEOBJECT_IO, "qt.remoteobjects.io", QtWarningMsg)

namespace QtRemoteObjects {

void copyStoredProperties(const QMetaObject *mo, const void *src, void *dst)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    if (!src) {
        qCWarning(QT_REMOTEOBJECT) << Q_FUNC_INFO << ": trying to copy from a null source";
        return;
    }
    if (!dst) {
        qCWarning(QT_REMOTEOBJECT) << Q_FUNC_INFO << ": trying to copy to a null destination";
        return;
    }

    for (int i = 0, end = mo->propertyCount(); i != end; ++i) {
        const QMetaProperty mp = mo->property(i);
        mp.writeOnGadget(dst, mp.readOnGadget(src));
    }
#else
    Q_UNUSED(mo);
    Q_UNUSED(src);
    Q_UNUSED(dst);
#endif
}

void copyStoredProperties(const QMetaObject *mo, const void *src, QDataStream &dst)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    if (!src) {
        qCWarning(QT_REMOTEOBJECT) << Q_FUNC_INFO << ": trying to copy from a null source";
        return;
    }

    for (int i = 0, end = mo->propertyCount(); i != end; ++i) {
        const QMetaProperty mp = mo->property(i);
        dst << mp.readOnGadget(src);
    }
#else
    Q_UNUSED(mo);
    Q_UNUSED(src);
    Q_UNUSED(dst);
#endif
}

void copyStoredProperties(const QMetaObject *mo, QDataStream &src, void *dst)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    if (!dst) {
        qCWarning(QT_REMOTEOBJECT) << Q_FUNC_INFO << ": trying to copy to a null destination";
        return;
    }

    for (int i = 0, end = mo->propertyCount(); i != end; ++i) {
        const QMetaProperty mp = mo->property(i);
        QVariant v;
        src >> v;
        mp.writeOnGadget(dst, v);
    }
#else
    Q_UNUSED(mo);
    Q_UNUSED(src);
    Q_UNUSED(dst);
#endif
}

} // namespace QtRemoteObjects

QT_END_NAMESPACE
