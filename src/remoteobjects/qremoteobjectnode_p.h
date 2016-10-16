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

#ifndef QREMOTEOBJECTNODE_P_H
#define QREMOTEOBJECTNODE_P_H

#include <QtCore/private/qobject_p.h>
#include "qremoteobjectsourceio_p.h"
#include "qremoteobjectreplica.h"
#include "qremoteobjectnode.h"

#include <QBasicTimer>
#include <QMutex>

QT_BEGIN_NAMESPACE

#define qRODebug(x) qCDebug(QT_REMOTEOBJECT) << qPrintable(QtPrivate::deref_for_methodcall(x).objectName())
#define qROWarning(x) qCWarning(QT_REMOTEOBJECT) << qPrintable(QtPrivate::deref_for_methodcall(x).objectName())
#define qROCritical(x) qCCritical(QT_REMOTEOBJECT) << qPrintable(QtPrivate::deref_for_methodcall(x).objectName())
#define qROFatal(x) qCFatal(QT_REMOTEOBJECT) << qPrintable(QtPrivate::deref_for_methodcall(x).objectName())
#define qROPrivDebug() qCDebug(QT_REMOTEOBJECT) << qPrintable(q_ptr->objectName())
#define qROPrivWarning() qCWarning(QT_REMOTEOBJECT) << qPrintable(q_ptr->objectName())
#define qROPrivCritical() qCCritical(QT_REMOTEOBJECT) << qPrintable(q_ptr->objectName())
#define qROPrivFatal() qCFatal(QT_REMOTEOBJECT) << qPrintable(q_ptr->objectName())

class QRemoteObjectRegistry;
class QRegistrySource;

class QRemoteObjectNodePrivate : public QObjectPrivate
{
public:
    QRemoteObjectNodePrivate();
    virtual ~QRemoteObjectNodePrivate();

    virtual QRemoteObjectSourceLocations remoteObjectAddresses() const;

    void setReplicaPrivate(const QMetaObject *, QRemoteObjectReplica *, const QString &);

    void setLastError(QRemoteObjectNode::ErrorCode errorCode);

    void connectReplica(QObject *object, QRemoteObjectReplica *instance);
    void openConnectionIfNeeded(const QString &name);

    bool initConnection(const QUrl &address);
    bool hasInstance(const QString &name);
    void setRegistry(QRemoteObjectRegistry *);

    void onClientRead(QObject *obj);
    void onRemoteObjectSourceAdded(const QRemoteObjectSourceLocation &entry);
    void onRemoteObjectSourceRemoved(const QRemoteObjectSourceLocation &entry);
    void onRegistryInitialized();
    void onShouldReconnect(ClientIoDevice *ioDevice);

    virtual QReplicaPrivateInterface *handleNewAcquire(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name);
    void initialize();
private:
    bool checkSignatures(const QByteArray &a, const QByteArray &b);

public:
    struct SourceInfo
    {
        ClientIoDevice* device;
        QString typeName;
        QByteArray objectSignature;
    };

    QAtomicInt isInitialized;
    QMutex mutex;
    QUrl registryAddress;
    QHash<QString, QWeakPointer<QReplicaPrivateInterface> > replicas;
    QMap<QString, SourceInfo> connectedSources;
    QSet<ClientIoDevice*> pendingReconnect;
    QSet<QUrl> requestedUrls;
    QSignalMapper clientRead;
    QRemoteObjectRegistry *registry;
    int retryInterval;
    QBasicTimer reconnectTimer;
    QRemoteObjectNode::ErrorCode lastError;
    QString rxName;
    QRemoteObjectPackets::ObjectInfoList rxObjects;
    QVariantList rxArgs;
    QVariant rxValue;
    QRemoteObjectPersistedStore *persistedStore;
    QRemoteObjectNode::StorageOwnership persistedStoreOwnership;
    Q_DECLARE_PUBLIC(QRemoteObjectNode)
};

class QRemoteObjectHostBasePrivate : public QRemoteObjectNodePrivate
{
public:
    QRemoteObjectHostBasePrivate();
    virtual ~QRemoteObjectHostBasePrivate() {}
    QReplicaPrivateInterface *handleNewAcquire(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name) Q_DECL_OVERRIDE;

public:
    QRemoteObjectSourceIo *remoteObjectIo;
    Q_DECLARE_PUBLIC(QRemoteObjectHostBase);
};

class QRemoteObjectHostPrivate : public QRemoteObjectHostBasePrivate
{
public:
    QRemoteObjectHostPrivate();
    virtual ~QRemoteObjectHostPrivate() {}
    Q_DECLARE_PUBLIC(QRemoteObjectHost);
};

class QRemoteObjectRegistryHostPrivate : public QRemoteObjectHostBasePrivate
{
public:
    QRemoteObjectRegistryHostPrivate();
    virtual ~QRemoteObjectRegistryHostPrivate() {}
    QRemoteObjectSourceLocations remoteObjectAddresses() const Q_DECL_OVERRIDE;
    QRegistrySource *registrySource;
    Q_DECLARE_PUBLIC(QRemoteObjectRegistryHost);
};

QT_END_NAMESPACE

#endif
