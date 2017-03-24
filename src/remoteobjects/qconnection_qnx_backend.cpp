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

#include "qconnection_qnx_backend_p.h"

QT_BEGIN_NAMESPACE

QnxClientIo::QnxClientIo(QObject *parent)
    : ClientIoDevice(parent)
{
    connect(&m_socket, &QQnxNativeIo::readyRead, this, &ClientIoDevice::readyRead);
    connect(&m_socket,
            static_cast<void(QQnxNativeIo::*)(QAbstractSocket::SocketError)>(&QQnxNativeIo::error),
            this,
            &QnxClientIo::onError);
    connect(&m_socket, &QQnxNativeIo::stateChanged, this, &QnxClientIo::onStateChanged);
}

QnxClientIo::~QnxClientIo()
{
    close();
}

QIODevice *QnxClientIo::connection()
{
    return &m_socket;
}

void QnxClientIo::doClose()
{
    if (m_socket.isOpen()) {
        connect(&m_socket, &QQnxNativeIo::disconnected, this, &QObject::deleteLater);
        m_socket.disconnectFromServer();
    } else {
        deleteLater();
    }
}

void QnxClientIo::connectToServer()
{
    if (!isOpen())
        m_socket.connectToServer(url().path());
}

bool QnxClientIo::isOpen()
{
    return !isClosing() && m_socket.isOpen();
}

void QnxClientIo::onError(QAbstractSocket::SocketError error)
{
    qCDebug(QT_REMOTEOBJECT) << "onError" << error << m_socket.serverName();

    switch (error) {
    case QAbstractSocket::RemoteHostClosedError:
        m_socket.close();
        qCWarning(QT_REMOTEOBJECT) << "RemoteHostClosedError";
    case QAbstractSocket::HostNotFoundError:     //Host not there, wait and try again
    case QAbstractSocket::AddressInUseError:
    case QAbstractSocket::ConnectionRefusedError:
        //... TODO error reporting
        emit shouldReconnect(this);
        break;
    default:
        break;
    }
}

void QnxClientIo::onStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ClosingState && !isClosing()) {
        m_socket.abort();
        emit shouldReconnect(this);
    } else if (state == QAbstractSocket::ConnectedState) {
        m_dataStream.setDevice(connection());
        m_dataStream.resetStatus();
    }
}

QnxServerIo::QnxServerIo(QIOQnxSource *conn, QObject *parent)
    : ServerIoDevice(parent), m_connection(conn)
{
    m_connection->setParent(this);
    connect(conn, &QIODevice::readyRead, this, &ServerIoDevice::readyRead);
    connect(conn, &QIOQnxSource::disconnected, this, &ServerIoDevice::disconnected);
}

QIODevice *QnxServerIo::connection() const
{
    return m_connection;
}

void QnxServerIo::doClose()
{
    m_connection->close();
}

QnxServerImpl::QnxServerImpl(QObject *parent)
    : QConnectionAbstractServer(parent)
{
    connect(&m_server,
            &QQnxNativeServer::newConnection,
            this,
            &QConnectionAbstractServer::newConnection);
}

QnxServerImpl::~QnxServerImpl()
{
    m_server.close();
}

ServerIoDevice *QnxServerImpl::configureNewConnection()
{
    if (!m_server.isListening())
        return Q_NULLPTR;

    return new QnxServerIo(m_server.nextPendingConnection(), this);
}

bool QnxServerImpl::hasPendingConnections() const
{
    return m_server.hasPendingConnections();
}

QUrl QnxServerImpl::address() const
{
    QUrl result;
    result.setPath(m_server.serverName());
    result.setScheme(QStringLiteral("qnx"));

    return result;
}

bool QnxServerImpl::listen(const QUrl &address)
{
    return m_server.listen(address.path());
}

QAbstractSocket::SocketError QnxServerImpl::serverError() const
{
    //TODO implement on QQnxNativeServer and here
    //return m_server.serverError();
    return QAbstractSocket::AddressInUseError;
}

void QnxServerImpl::close()
{
    close();
}

REGISTER_QTRO_CLIENT(QnxClientIo, "qnx");
REGISTER_QTRO_SERVER(QnxServerImpl, "qnx");

QT_END_NAMESPACE
