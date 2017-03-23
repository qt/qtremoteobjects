/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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

#include "qconnectionfactories.h"
#include "qtremoteobjectglobal.h"
#include "qremoteobjectpacket_p.h"

#include <QIODevice>
#include <QScopedPointer>
#include <QSignalMapper>

QT_BEGIN_NAMESPACE

class QRemoteObjectSource;
class SourceApiMap;

class QRemoteObjectSourceIo : public QObject
{
    Q_OBJECT
public:
    explicit QRemoteObjectSourceIo(const QUrl &address, QObject *parent = nullptr);
    ~QRemoteObjectSourceIo();

    bool enableRemoting(QObject *object, const QMetaObject *meta, const QString &name, const QString &typeName);
    bool enableRemoting(QObject *object, const SourceApiMap *api, QObject *adapter = nullptr);
    bool disableRemoting(QObject *object);

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
    void registerSource(QRemoteObjectSource *pp);
    void unregisterSource(QRemoteObjectSource *pp);

    QHash<QIODevice*, quint32> m_readSize;
    QSet<ServerIoDevice*> m_connections;
    QHash<QObject *, QRemoteObjectSource*> m_objectToSourceMap;
    QMap<QString, QRemoteObjectSource*> m_remoteObjects;
    QSignalMapper m_serverDelete;
    QSignalMapper m_serverRead;
    QHash<ServerIoDevice*, QUrl> m_registryMapping;
    QScopedPointer<QConnectionAbstractServer> m_server;
    QRemoteObjectPackets::DataStreamPacket m_packet;
    QString m_rxName;
    QVariantList m_rxArgs;
};

QT_END_NAMESPACE

#endif
