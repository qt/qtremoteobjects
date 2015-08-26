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

#include "qconnectionclientfactory_p.h"

#include "qremoteobjectpacket_p.h"
#include "qtremoteobjectglobal.h"

#include <QDataStream>
#include <QHostAddress>
#include <QHostInfo>

QT_BEGIN_NAMESPACE

ClientIoDevice::ClientIoDevice(QObject *parent)
    : QObject(parent), m_isClosing(false), m_curReadSize(0), m_packet(Q_NULLPTR)
{
    m_dataStream.setVersion(QRemoteObjectPackets::dataStreamVersion);
}

ClientIoDevice::~ClientIoDevice()
{
    delete m_packet;
}

void ClientIoDevice::close()
{
    m_isClosing = true;
    doClose();
}

bool ClientIoDevice::read()
{
    qCDebug(QT_REMOTEOBJECT) << "ClientIODevice::read()" << m_curReadSize << bytesAvailable();

    if (m_curReadSize == 0) {
        if (bytesAvailable() < static_cast<int>(sizeof(quint32)))
            return false;

        m_dataStream >> m_curReadSize;
    }

    qCDebug(QT_REMOTEOBJECT) << "ClientIODevice::read()-looking for map" << m_curReadSize << bytesAvailable();

    if (bytesAvailable() < m_curReadSize)
        return false;

    m_curReadSize = 0;
    delete m_packet;
    m_packet = QRemoteObjectPackets::QRemoteObjectPacket::fromDataStream(m_dataStream);
    return m_packet && m_packet->id != QRemoteObjectPackets::QRemoteObjectPacket::Invalid;
}

void ClientIoDevice::write(const QByteArray &data)
{
    connection()->write(data);
}

void ClientIoDevice::write(const QByteArray &data, qint64 size)
{
    connection()->write(data.data(), size);
}

qint64 ClientIoDevice::bytesAvailable()
{
    return connection()->bytesAvailable();
}

QRemoteObjectPackets::QRemoteObjectPacket *ClientIoDevice::packet() const
{
    return m_packet;
}

QUrl ClientIoDevice::url() const
{
    return m_url;
}

void ClientIoDevice::addSource(const QString &name)
{
    m_remoteObjects.insert(name);
}

void ClientIoDevice::removeSource(const QString &name)
{
    m_remoteObjects.remove(name);
}

QSet<QString> ClientIoDevice::remoteObjects() const
{
    return m_remoteObjects;
}


LocalClientIo::LocalClientIo(QObject *parent)
    : ClientIoDevice(parent)
{
    connect(&m_socket, &QLocalSocket::readyRead, this, &ClientIoDevice::readyRead);
    connect(&m_socket, static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this, &LocalClientIo::onError);
    connect(&m_socket, &QLocalSocket::stateChanged, this, &LocalClientIo::onStateChanged);
}

LocalClientIo::~LocalClientIo()
{
    close();
}

QIODevice *LocalClientIo::connection()
{
    return &m_socket;
}

void LocalClientIo::doClose()
{
    if (m_socket.isOpen()) {
        connect(&m_socket, &QLocalSocket::disconnected, this, &QObject::deleteLater);
        m_socket.disconnectFromServer();
    } else {
        this->deleteLater();
    }
}

void LocalClientIo::connectToServer()
{
    if (!isOpen())
        m_socket.connectToServer(url().path());
}

bool LocalClientIo::isOpen()
{
    return !isClosing() && m_socket.isOpen();
}

void LocalClientIo::onError(QLocalSocket::LocalSocketError error)
{
    qCDebug(QT_REMOTEOBJECT) << "onError" << error << m_socket.serverName();

    switch (error) {
    case QLocalSocket::ServerNotFoundError:     //Host not there, wait and try again
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
        m_socket.abort();
        emit shouldReconnect(this);
    }
    if (state == QLocalSocket::ConnectedState) {
        m_dataStream.setDevice(connection());
        m_dataStream.resetStatus();
    }
}


TcpClientIo::TcpClientIo(QObject *parent)
    : ClientIoDevice(parent)
{
    connect(&m_socket, &QTcpSocket::readyRead, this, &ClientIoDevice::readyRead);
    connect(&m_socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &TcpClientIo::onError);
    connect(&m_socket, &QTcpSocket::stateChanged, this, &TcpClientIo::onStateChanged);
}

TcpClientIo::~TcpClientIo()
{
    close();
}

QIODevice *TcpClientIo::connection()
{
    return &m_socket;
}

void TcpClientIo::doClose()
{
    if (m_socket.isOpen()) {
        connect(&m_socket, &QTcpSocket::disconnected, this, &QObject::deleteLater);
        m_socket.disconnectFromHost();
    } else {
        this->deleteLater();
    }
}

void TcpClientIo::connectToServer()
{
    if (isOpen())
        return;
    QHostAddress address(url().host());
    if (address.isNull()) {
        const QList<QHostAddress> addresses = QHostInfo::fromName(url().host()).addresses();
        Q_ASSERT_X(addresses.size() >= 1, Q_FUNC_INFO, url().toString().toLatin1().data());
        address = addresses.first();
    }

    m_socket.connectToHost(address, url().port());
}

bool TcpClientIo::isOpen()
{
    return (!isClosing() && m_socket.isOpen());
}

void TcpClientIo::onError(QAbstractSocket::SocketError error)
{
    qCDebug(QT_REMOTEOBJECT) << "onError" << error;

    switch (error) {
    case QAbstractSocket::HostNotFoundError:     //Host not there, wait and try again
        emit shouldReconnect(this);
        break;
    case QAbstractSocket::AddressInUseError:
    case QAbstractSocket::ConnectionRefusedError:
        //... TODO error reporting
        break;
    default:
        break;
    }
}

void TcpClientIo::onStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ClosingState && !isClosing()) {
        m_socket.abort();
        emit shouldReconnect(this);
    }
    if (state == QAbstractSocket::ConnectedState) {
        m_dataStream.setDevice(connection());
        m_dataStream.resetStatus();
    }
}


QConnectionClientFactory::QConnectionClientFactory()
{
    registerProduct<LocalClientIo>(QRemoteObjectStringLiterals::local());
    registerProduct<TcpClientIo>(QRemoteObjectStringLiterals::tcp());
}

ClientIoDevice *QConnectionClientFactory::createDevice(const QUrl &url, QObject *parent)
{
    ClientIoDevice *res = QConnectionAbstractFactory<ClientIoDevice>::create(url.scheme(), parent);
    res->m_url = url;
    return res;
}

QT_END_NAMESPACE
