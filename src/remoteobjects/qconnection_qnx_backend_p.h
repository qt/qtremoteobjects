/****************************************************************************
**
** Copyright (C) 2014-2016 Ford Motor Company
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

#ifndef QCONNECTIONQNXBACKEND_P_H
#define QCONNECTIONQNXBACKEND_P_H

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

#include <QtRemoteObjects/qconnectionfactories.h>
#include "qconnection_qnx_qiodevices.h"
#include "qconnection_qnx_server.h"

QT_BEGIN_NAMESPACE

/*!
    QtRO provides ClientIoDevice, ServerIoDevice and QConnectionAbstractServer
    as abstract interfaces to allow different backends to be used by QtRO. The
    concept behind these classes is that there needs to be a Host node, which
    has an address that can be connected to. Then there is a client object,
    which can be publicly constructed, and can connect to the server. When the
    server gets a connection request, it creates the server side of the
    connection, which communicates directly with the client. There are thus
    three abstractions, one for the server, one for the client-side of the
    connection, and the third for the server-side of the connection. The later
    two need to inherit from QIODevice.

    Creating a backend for something that is already implemented in Qt is a
    matter of creating the three needed abstractions. In the case of creating a
    QNX backend using QNX's Native Messaging, the backend needs to create the
    Server (which has an address for accepting connections), the client
    QIODevice, and the server side QIODevice. Since Native Messaging is one
    way, and recommends using pulses to support two-way communication, the
    logic for the client-side and server-side QIODevice are very different.
    Thus, three additional backend classes are needed as well.

    QnxClientIo implements the QtRO ClientIoDevice wrapper around the QNX
    specific QQnxNativeIo QIODevice (the client-side QIODevice).

    QnxServerIo implements the QtRO ServerIoDevice wrapper around the QNX
    specific QIOQnxSource QIODevice (the server-side QIODevice).

    QnxServerImpl implements the QtRO QConnectionAbstractServer wrapper around
    the QNX specific QQnxNativeServer, which is the server object listening for
    connections.

    Not sure if it is of interest to the Qt community, but it seems like
    QQnxNativeIo, QIOQnxSource and QQnxNativeServer could used as an optimized
    QLocalServer/QLocalSocket QPA for QNX.
*/

class QnxClientIo : public ClientIoDevice
{
    Q_OBJECT

public:
    explicit QnxClientIo(QObject *parent = Q_NULLPTR);
    ~QnxClientIo();

    QIODevice *connection() Q_DECL_OVERRIDE;
    void connectToServer() Q_DECL_OVERRIDE;
    bool isOpen() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void onError(QAbstractSocket::SocketError error);
    void onStateChanged(QAbstractSocket::SocketState state);

protected:
    void doClose() Q_DECL_OVERRIDE;
private:
    QQnxNativeIo m_socket;
};

class QnxServerIo : public ServerIoDevice
{
public:
    explicit QnxServerIo(QIOQnxSource *conn, QObject *parent = Q_NULLPTR);

    QIODevice *connection() const Q_DECL_OVERRIDE;
protected:
    void doClose() Q_DECL_OVERRIDE;

private:
    //TODO Source or Replica
    QIOQnxSource *m_connection;
};

class QnxServerImpl : public QConnectionAbstractServer
{
    Q_OBJECT

public:
    explicit QnxServerImpl(QObject *parent);
    ~QnxServerImpl();

    bool hasPendingConnections() const Q_DECL_OVERRIDE;
    ServerIoDevice *configureNewConnection() Q_DECL_OVERRIDE;
    QUrl address() const Q_DECL_OVERRIDE;
    bool listen(const QUrl &address) Q_DECL_OVERRIDE;
    QAbstractSocket::SocketError serverError() const Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

private:
    QQnxNativeServer m_server;
};

QT_END_NAMESPACE

#endif // QCONNECTIONQNXBACKEND_P_H

