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
******************************************************************************/

#include "qremoteobjectsettingsstore.h"

#include "qremoteobjectnode_p.h"

#include <QtCore/private/qobject_p.h>
#include <QtCore/qsettings.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SettingsStore
    \inqmlmodule QtRemoteObjects
    \brief A basic store for persisted properties.

    This type provides simple QSettings-based storage for properties marked as PERSISTED. It is used in
    conjunction with Node::persistedStore:

    \code
    Node {
        persistedStore: SettingsStore {}
    }
    \endcode
*/

class QRemoteObjectSettingsStorePrivate : public QRemoteObjectAbstractPersistedStorePrivate
{
public:
    QRemoteObjectSettingsStorePrivate();
    virtual ~QRemoteObjectSettingsStorePrivate();

    QSettings settings;
    Q_DECLARE_PUBLIC(QRemoteObjectSettingsStore)
};

QRemoteObjectSettingsStorePrivate::QRemoteObjectSettingsStorePrivate()
{
}

QRemoteObjectSettingsStorePrivate::~QRemoteObjectSettingsStorePrivate()
{
}

QRemoteObjectSettingsStore::QRemoteObjectSettingsStore(QObject *parent)
    : QRemoteObjectAbstractPersistedStore(*new QRemoteObjectSettingsStorePrivate, parent)
{
}

QRemoteObjectSettingsStore::~QRemoteObjectSettingsStore()
{
}

QVariantList QRemoteObjectSettingsStore::restoreProperties(const QString &repName, const QByteArray &repSig)
{
    Q_D(QRemoteObjectSettingsStore);
    d->settings.beginGroup(repName + QLatin1Char('/') + QString::fromLatin1(repSig));
    QVariantList values = d->settings.value(QStringLiteral("values")).toList();
    d->settings.endGroup();
    return values;
}

void QRemoteObjectSettingsStore::saveProperties(const QString &repName, const QByteArray &repSig, const QVariantList &values)
{
    Q_D(QRemoteObjectSettingsStore);
    d->settings.beginGroup(repName + QLatin1Char('/') + QString::fromLatin1(repSig));
    d->settings.setValue(QStringLiteral("values"), values);
    d->settings.endGroup();
    d->settings.sync();
}

QT_END_NAMESPACE
