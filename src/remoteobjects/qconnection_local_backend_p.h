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

#ifndef QCONNECTIONCLIENTFACTORY_P_H
#define QCONNECTIONCLIENTFACTORY_P_H

#include "qconnectionfactories.h"

#include <QLocalServer>
#include <QLocalSocket>

QT_BEGIN_NAMESPACE

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

class LocalServerIo : public ServerIoDevice
{
public:
    explicit LocalServerIo(QLocalSocket *conn, QObject *parent = Q_NULLPTR);

    QIODevice *connection() const Q_DECL_OVERRIDE;
protected:
    void doClose() Q_DECL_OVERRIDE;

private:
    QLocalSocket *m_connection;
};

class LocalServerImpl : public QConnectionAbstractServer
{
    Q_OBJECT
    Q_DISABLE_COPY(LocalServerImpl)

public:
    explicit LocalServerImpl(QObject *parent);
    ~LocalServerImpl();

    bool hasPendingConnections() const Q_DECL_OVERRIDE;
    ServerIoDevice *_nextPendingConnection() Q_DECL_OVERRIDE;
    QUrl address() const Q_DECL_OVERRIDE;
    bool listen(const QUrl &address) Q_DECL_OVERRIDE;
    QAbstractSocket::SocketError serverError() const Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

private:
    QLocalServer m_server;
};

QT_END_NAMESPACE

#endif
