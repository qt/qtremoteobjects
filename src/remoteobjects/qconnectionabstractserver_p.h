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

#ifndef QCONNECTIONABSTRACTSERVER_P_H
#define QCONNECTIONABSTRACTSERVER_P_H

#include <QAbstractSocket>
#include <QIODevice>
#include <QObject>
#include <QDataStream>
#include "qtremoteobjectglobal.h"
#include "qremoteobjectpacket_p.h"

QT_BEGIN_NAMESPACE

//The Qt servers create QIODevice derived classes from handleConnection.
//The problem is that they behave differently, so this class adds some
//consistency.
class ServerIoDevice : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ServerIoDevice)

public:
    explicit ServerIoDevice(QObject *parent = Q_NULLPTR);
    virtual ~ServerIoDevice();

    bool read(QRemoteObjectPackets::QRemoteObjectPacketTypeEnum &, QString &);

    virtual void write(const QByteArray &data);
    virtual void write(const QByteArray &data, qint64);
    void close();
    virtual qint64 bytesAvailable();
    virtual QIODevice *connection() const = 0;
    void initializeDataStream();
    QDataStream& stream() { return m_dataStream; }

Q_SIGNALS:
    void disconnected();
    void readyRead();

protected:
    virtual void doClose() = 0;

private:
    bool m_isClosing;
    quint32 m_curReadSize;
    QDataStream m_dataStream;
};


class QConnectionAbstractServer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QConnectionAbstractServer)

public:
    explicit QConnectionAbstractServer(QObject *parent = Q_NULLPTR);
    virtual ~QConnectionAbstractServer();

    virtual bool hasPendingConnections() const = 0;
    ServerIoDevice* nextPendingConnection();
    virtual QUrl address() const = 0;
    virtual bool listen(const QUrl &address) = 0;
    virtual QAbstractSocket::SocketError serverError() const = 0;
    virtual void close() = 0;

protected:
    virtual ServerIoDevice* _nextPendingConnection() = 0;

Q_SIGNALS:
    void newConnection();
};

QT_END_NAMESPACE

#endif
