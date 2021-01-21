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

#ifndef QREMOTEOBJECTSOURCEIO_P_H
#define QREMOTEOBJECTSOURCEIO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qconnectionfactories_p.h"
#include "qtremoteobjectglobal.h"
#include "qremoteobjectpacket_p.h"

#include <QtCore/qiodevice.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QRemoteObjectSourceBase;
class QRemoteObjectRootSource;
class SourceApiMap;
class QRemoteObjectHostBase;

class QRemoteObjectSourceIo : public QObject
{
    Q_OBJECT
public:
    explicit QRemoteObjectSourceIo(const QUrl &address, QObject *parent = nullptr);
    explicit QRemoteObjectSourceIo(QObject *parent = nullptr);
    ~QRemoteObjectSourceIo() override;

    bool startListening();
    bool enableRemoting(QObject *object, const QMetaObject *meta, const QString &name,
                        const QString &typeName);
    bool enableRemoting(QObject *object, const SourceApiMap *api, QObject *adapter = nullptr);
    bool disableRemoting(QObject *object);
    void newConnection(IoDeviceBase *conn);

    QUrl serverAddress() const;

public Q_SLOTS:
    void handleConnection();
    void onServerDisconnect(QObject *obj = nullptr);
    void onServerRead(QObject *obj);

Q_SIGNALS:
    void remoteObjectAdded(const QRemoteObjectSourceLocation &);
    void remoteObjectRemoved(const QRemoteObjectSourceLocation &);
    void serverRemoved(const QUrl& url);

public:
    void registerSource(QRemoteObjectSourceBase *source);
    void unregisterSource(QRemoteObjectSourceBase *source);

    QHash<QIODevice*, quint32> m_readSize;
    QSet<IoDeviceBase*> m_connections;
    QHash<QObject *, QRemoteObjectRootSource*> m_objectToSourceMap;
    QMap<QString, QRemoteObjectSourceBase*> m_sourceObjects;
    QMap<QString, QRemoteObjectRootSource*> m_sourceRoots;
    QHash<IoDeviceBase*, QUrl> m_registryMapping;
    QScopedPointer<QConnectionAbstractServer> m_server;
    QRemoteObjectPackets::DataStreamPacket m_packet;
    QString m_rxName;
    QVariantList m_rxArgs;
    QUrl m_address;
};

QT_END_NAMESPACE

#endif
