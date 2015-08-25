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

#ifndef QCONNECTIONCLIENTFACTORY_P_H
#define QCONNECTIONCLIENTFACTORY_P_H

#include "qconnectionabstractfactory_p.h"
#include "qtremoteobjectglobal.h"

#include <QLocalSocket>
#include <QObject>
#include <QTcpSocket>
#include <QUrl>
#include <QDataStream>

QT_BEGIN_NAMESPACE

namespace QRemoteObjectPackets {
class QRemoteObjectPacket;
}

class ClientIoDevice : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ClientIoDevice)

public:
    explicit ClientIoDevice(QObject *parent = Q_NULLPTR);
    virtual ~ClientIoDevice();

    QRemoteObjectPackets::QRemoteObjectPacket* read();
    virtual void write(const QByteArray &data);
    virtual void write(const QByteArray &data, qint64);
    void close();
    virtual void connectToServer() = 0;
    virtual qint64 bytesAvailable();

    QRemoteObjectPackets::QRemoteObjectPacket *packet() const;
    QUrl url() const;
    void addSource(const QString &);
    void removeSource(const QString &);
    QSet<QString> remoteObjects() const;

    virtual bool isOpen() = 0;
    virtual QIODevice *connection() = 0;

Q_SIGNALS:
    void disconnected();
    void readyRead();
    void shouldReconnect(ClientIoDevice*);
protected:
    virtual void doClose() = 0;
    inline bool isClosing();
    QDataStream m_dataStream;

private:
    bool m_isClosing;
    QUrl m_url;

private:
    friend class QConnectionClientFactory;

    quint32 m_curReadSize;
    QRemoteObjectPackets::QRemoteObjectPacket* m_packet;
    QSet<QString> m_remoteObjects;
    QVector<QRemoteObjectPackets::QRemoteObjectPacket*> m_packetStorage;
};

bool ClientIoDevice::isClosing()
{
    return m_isClosing;
}

class LocalClientIo : public ClientIoDevice
{
    Q_OBJECT

public:
    explicit LocalClientIo(QObject *parent = Q_NULLPTR);
    ~LocalClientIo();

    QIODevice *connection() Q_DECL_OVERRIDE;
    void connectToServer() Q_DECL_OVERRIDE;
    bool isOpen() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void onError(QLocalSocket::LocalSocketError error);
    void onStateChanged(QLocalSocket::LocalSocketState state);

protected:
    void doClose() Q_DECL_OVERRIDE;
private:
    QLocalSocket m_socket;
};


class TcpClientIo : public ClientIoDevice
{
    Q_OBJECT

public:
    explicit TcpClientIo(QObject *parent = Q_NULLPTR);
    ~TcpClientIo();

    QIODevice *connection() Q_DECL_OVERRIDE;
    void connectToServer() Q_DECL_OVERRIDE;
    bool isOpen() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void onError(QAbstractSocket::SocketError error);
    void onStateChanged(QAbstractSocket::SocketState state);

protected:
    void doClose() Q_DECL_OVERRIDE;

private:
    QTcpSocket m_socket;
};

class QConnectionClientFactory : public QConnectionAbstractFactory<ClientIoDevice>
{
    Q_DISABLE_COPY(QConnectionClientFactory)

public:
    QConnectionClientFactory();

    ClientIoDevice *createDevice(const QUrl &url, QObject *parent = Q_NULLPTR);
};

QT_END_NAMESPACE

#endif
