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

#ifndef QREMOTEOBJECTREGISTRY_P_H
#define QREMOTEOBJECTREGISTRY_P_H

#include <QtRemoteObjects/qremoteobjectreplica.h>

QT_BEGIN_NAMESPACE

class QRemoteObjectRegistryPrivate;

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectRegistry : public QRemoteObjectReplica
{
    Q_OBJECT
    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, "Registry")

    Q_PROPERTY(QRemoteObjectSourceLocations sourceLocations READ sourceLocations)

public:
    ~QRemoteObjectRegistry() override;
    static void registerMetatypes();

    QRemoteObjectSourceLocations sourceLocations() const;

Q_SIGNALS:
    void remoteObjectAdded(const QRemoteObjectSourceLocation &entry);
    void remoteObjectRemoved(const QRemoteObjectSourceLocation &entry);

protected Q_SLOTS:
    void addSource(const QRemoteObjectSourceLocation &entry);
    void removeSource(const QRemoteObjectSourceLocation &entry);
    void pushToRegistryIfNeeded();

private:
    void initialize() override;

    explicit QRemoteObjectRegistry(QObject *parent = nullptr);
    explicit QRemoteObjectRegistry(QRemoteObjectNode *node, const QString &name, QObject *parent = nullptr);

    Q_DECLARE_PRIVATE(QRemoteObjectRegistry)
    friend class QT_PREPEND_NAMESPACE(QRemoteObjectNode);
};

QT_END_NAMESPACE

#endif
