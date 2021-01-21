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

#ifndef QREMOTEOBJECTSETTINGSSTORE_H
#define QREMOTEOBJECTSETTINGSSTORE_H

#include <QtRemoteObjects/qremoteobjectnode.h>

QT_BEGIN_NAMESPACE

class QRemoteObjectSettingsStorePrivate;

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectSettingsStore : public QRemoteObjectAbstractPersistedStore
{
    Q_OBJECT

public:
    QRemoteObjectSettingsStore(QObject *parent = nullptr);
    ~QRemoteObjectSettingsStore() override;

    void saveProperties(const QString &repName, const QByteArray &repSig, const QVariantList &values) override;
    QVariantList restoreProperties(const QString &repName, const QByteArray &repSig) override;

private:
    Q_DECLARE_PRIVATE(QRemoteObjectSettingsStore)
};

QT_END_NAMESPACE

#endif // QREMOTEOBJECTSETTINGSSTORE_H
