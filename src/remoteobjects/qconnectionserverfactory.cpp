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

#include "qconnectionserverfactory_p.h"

#include <qcompilerdetection.h>
#include <QHostInfo>
#include <QIODevice>
#include <QLocalServer>
#include <QTcpServer>
#include <QtGlobal>
#include <QLocalSocket>
#include <QTcpSocket>

#ifdef Q_OS_LINUX
#include <QFile>
#include <QDir>
#endif

QT_BEGIN_NAMESPACE

LocalServerIo::LocalServerIo(QLocalSocket *conn, QObject *parent)
    : ServerIoDevice(parent), m_connection(conn)
{
    m_connection->setParent(this);
    connect(conn, &QIODevice::readyRead, this, &ServerIoDevice::readyRead);
    connect(conn, &QLocalSocket::disconnected, this, &ServerIoDevice::disconnected);
}

QIODevice *LocalServerIo::connection() const
{
    return m_connection;
}

void LocalServerIo::doClose()
{
    m_connection->disconnectFromServer();
}


TcpServerIo::TcpServerIo(QTcpSocket *conn, QObject *parent)
    : ServerIoDevice(parent), m_connection(conn)
{
    m_connection->setParent(this);
    connect(conn, &QIODevice::readyRead, this, &ServerIoDevice::readyRead);
    connect(conn, &QAbstractSocket::disconnected, this, &ServerIoDevice::disconnected);
}

QIODevice *TcpServerIo::connection() const
{
    return m_connection;
}

void TcpServerIo::doClose()
{
    m_connection->disconnectFromHost();
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

ServerIoDevice *LocalServerImpl::_nextPendingConnection()
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
    close();
}


TcpServerImpl::TcpServerImpl(QObject *parent)
    : QConnectionAbstractServer(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &QConnectionAbstractServer::newConnection);
}

TcpServerImpl::~TcpServerImpl()
{
    close();
}

ServerIoDevice *TcpServerImpl::_nextPendingConnection()
{
    if (!m_server.isListening())
        return Q_NULLPTR;

    return new TcpServerIo(m_server.nextPendingConnection());
}

bool TcpServerImpl::hasPendingConnections() const
{
    return m_server.hasPendingConnections();
}

QUrl TcpServerImpl::address() const
{
    return m_originalUrl;
}

bool TcpServerImpl::listen(const QUrl &address)
{
    QHostAddress host(address.host());
    if (host.isNull())
        host = QHostAddress::Any;

    bool ret = m_server.listen(host, address.port());
    if (ret) {
        m_originalUrl.setScheme(QLatin1String("tcp"));
        m_originalUrl.setHost(m_server.serverAddress().toString());
        m_originalUrl.setPort(m_server.serverPort());
    }
    return ret;
}

QAbstractSocket::SocketError TcpServerImpl::serverError() const
{
    return m_server.serverError();
}

void TcpServerImpl::close()
{
    m_server.close();
}

QConnectionServerFactory::QConnectionServerFactory()
{
    registerProduct<LocalServerImpl>(QRemoteObjectStringLiterals::local());
    registerProduct<TcpServerImpl>(QRemoteObjectStringLiterals::tcp());
}

QConnectionAbstractServer *QConnectionServerFactory::createServer(const QUrl &url, QObject *parent)
{
    return create(url, parent);
}

QConnectionAbstractServer *QConnectionServerFactory::create(const QUrl &url, QObject *parent)
{
    return QConnectionAbstractFactory<QConnectionAbstractServer>::create(url.scheme(), parent);
}

QT_END_NAMESPACE
