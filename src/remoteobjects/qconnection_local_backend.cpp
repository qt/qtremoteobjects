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

#include "qconnection_local_backend_p.h"

LocalClientIo::LocalClientIo(QObject *parent)
    : ClientIoDevice(parent)
{
    m_socket = QSharedPointer<QLocalSocket>(new QLocalSocket());
    connect(m_socket.data(), &QLocalSocket::readyRead, this, &ClientIoDevice::readyRead);
    connect(m_socket.data(), static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this, &LocalClientIo::onError);
    connect(m_socket.data(), &QLocalSocket::stateChanged, this, &LocalClientIo::onStateChanged);
}

LocalClientIo::LocalClientIo(QSharedPointer<QLocalSocket> socket, QObject *parent)
    : ClientIoDevice(parent)
{
    m_socket = socket;
    connect(m_socket.data(), &QLocalSocket::readyRead, this, &ClientIoDevice::readyRead);
    connect(m_socket.data(), static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this, &LocalClientIo::onError);
    connect(m_socket.data(), &QLocalSocket::stateChanged, this, &LocalClientIo::onStateChanged);
    onStateChanged(m_socket->state());
}

LocalClientIo::~LocalClientIo()
{
    close();
}

QSharedPointer<QIODevice> LocalClientIo::connection()
{
    return m_socket;
}

void LocalClientIo::doClose()
{
    if (m_socket.data()->isOpen()) {
        connect(m_socket.data(), &QLocalSocket::disconnected, this, &QObject::deleteLater);
        m_socket.data()->disconnectFromServer();
    } else {
        this->deleteLater();
    }
}

void LocalClientIo::connectToServer()
{
    if (!isOpen())
        m_socket.data()->connectToServer(url().path());
}

bool LocalClientIo::isOpen()
{
    return !isClosing() && m_socket.data()->isOpen();
}

void LocalClientIo::onError(QLocalSocket::LocalSocketError error)
{
    qCDebug(QT_REMOTEOBJECT) << "onError" << error << m_socket.data()->serverName();

    switch (error) {
    case QLocalSocket::ServerNotFoundError:
    case QLocalSocket::UnknownSocketError:
        //Host not there, wait and try again
        emit shouldReconnect(this);
        break;
    case QLocalSocket::ConnectionError:
    case QLocalSocket::ConnectionRefusedError:
        //... TODO error reporting
#ifdef Q_OS_UNIX
        emit shouldReconnect(this);
#endif
        break;
    default:
        break;
    }
}

void LocalClientIo::onStateChanged(QLocalSocket::LocalSocketState state)
{
    if (state == QLocalSocket::ClosingState && !isClosing()) {
        m_socket.data()->abort();
        emit shouldReconnect(this);
    }
    if (state == QLocalSocket::ConnectedState) {
        m_dataStream.setDevice(connection().data());
        m_dataStream.resetStatus();
    }
}

LocalServerIo::LocalServerIo(QLocalSocket *conn, QObject *parent)
    : ServerIoDevice(parent), m_connection(QSharedPointer<QLocalSocket>(conn))
{
    connect(conn, &QIODevice::readyRead, this, &ServerIoDevice::readyRead);
    connect(conn, &QLocalSocket::disconnected, this, &ServerIoDevice::disconnected);
}

LocalServerIo::LocalServerIo(QSharedPointer<QLocalSocket> conn, QObject *parent)
    : ServerIoDevice(parent), m_connection(conn)
{
    connect(m_connection.data(), &QIODevice::readyRead, this, &ServerIoDevice::readyRead);
    connect(m_connection.data(), &QLocalSocket::disconnected, this, &ServerIoDevice::disconnected);
    initializeDataStream();
}

QSharedPointer<QIODevice> LocalServerIo::connection() const
{
    return m_connection;
}

void LocalServerIo::doClose()
{
    m_connection->disconnectFromServer();
}

LocalServerImpl::LocalServerImpl(QObject *parent)
    : QConnectionAbstractServer(parent)
{
    connect(&m_server, &QLocalServer::newConnection, this, &QConnectionAbstractServer::newConnection);
}

LocalServerImpl::~LocalServerImpl()
{
    m_server.close();
}

ServerIoDevice *LocalServerImpl::configureNewConnection()
{
    if (!m_server.isListening())
        return Q_NULLPTR;

    return new LocalServerIo(m_server.nextPendingConnection(), this);
}

bool LocalServerImpl::hasPendingConnections() const
{
    return m_server.hasPendingConnections();
}

QUrl LocalServerImpl::address() const
{
    QUrl result;
    result.setPath(m_server.serverName());
    result.setScheme(QRemoteObjectStringLiterals::local());

    return result;
}

bool LocalServerImpl::listen(const QUrl &address)
{
#ifdef Q_OS_UNIX
    bool res = m_server.listen(address.path());
    if (!res) {
        QLocalServer::removeServer(address.path());
        res = m_server.listen(address.path());
    }
    return res;
#else
    return m_server.listen(address.path());
#endif
}

QAbstractSocket::SocketError LocalServerImpl::serverError() const
{
    return m_server.serverError();
}

void LocalServerImpl::close()
{
    m_server.close();
}

REGISTER_QTRO_SERVER(LocalServerImpl, "local");
REGISTER_QTRO_CLIENT(LocalClientIo, "local");

QT_END_NAMESPACE
