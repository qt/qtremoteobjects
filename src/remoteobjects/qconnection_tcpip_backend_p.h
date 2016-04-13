/****************************************************************************
**
** Copyright (C) 2014-2015 Ford Motor Company
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

#ifndef QCONNECTIONTCPIPBACKEND_P_H
#define QCONNECTIONTCPIPBACKEND_P_H

#include "qconnectionfactories.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE

class TcpClientIo : public ClientIoDevice
{
    Q_OBJECT

public:
    explicit TcpClientIo(QObject *parent = Q_NULLPTR);
    TcpClientIo(QSharedPointer<QTcpSocket> socket, QObject *parent = Q_NULLPTR);
    ~TcpClientIo();

    QSharedPointer<QIODevice> connection() Q_DECL_OVERRIDE;
    void connectToServer() Q_DECL_OVERRIDE;
    bool isOpen() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void onError(QAbstractSocket::SocketError error);
    void onStateChanged(QAbstractSocket::SocketState state);

protected:
    void doClose() Q_DECL_OVERRIDE;

private:
    QSharedPointer<QTcpSocket> m_socket;
};

class TcpServerIo : public ServerIoDevice
{
    Q_OBJECT
public:
    explicit TcpServerIo(QTcpSocket *conn, QObject *parent = Q_NULLPTR);
    TcpServerIo(QSharedPointer<QTcpSocket> conn, QObject *parent = Q_NULLPTR);

    QSharedPointer<QIODevice> connection() const Q_DECL_OVERRIDE;
protected:
    void doClose() Q_DECL_OVERRIDE;

private:
    QSharedPointer<QTcpSocket> m_connection;
};

class TcpServerImpl : public QConnectionAbstractServer
{
    Q_OBJECT
    Q_DISABLE_COPY(TcpServerImpl)

public:
    explicit TcpServerImpl(QObject *parent);
    ~TcpServerImpl();

    bool hasPendingConnections() const Q_DECL_OVERRIDE;
    ServerIoDevice *configureNewConnection() Q_DECL_OVERRIDE;
    QUrl address() const Q_DECL_OVERRIDE;
    bool listen(const QUrl &address) Q_DECL_OVERRIDE;
    QAbstractSocket::SocketError serverError() const Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

private:
    QTcpServer m_server;
    QUrl m_originalUrl; // necessary because of a QHostAddress bug
};

QT_END_NAMESPACE
#endif // QCONNECTIONTCPIPBACKEND_P_H
