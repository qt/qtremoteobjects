// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
