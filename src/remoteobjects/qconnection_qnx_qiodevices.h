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

#ifndef QQNXNATIVEIO_H
#define QQNXNATIVEIO_H

#include <QAbstractSocket>
#include <QtRemoteObjects/qtremoteobjectglobal.h>

QT_BEGIN_NAMESPACE

/*
 * The implementation of the Source and Replica
 * side QIODevice look like they will be fairly
 * different, including different APIs.  So
 * creating a 2nd derived type for the source.
 *
 * TODO: revisit if these can be combined into a
 * single type.
 *
 * With two types, QQnxNativeIo will need to get
 * Source or Replica added.  Not sure what intuitive
 * names are yet.  So for now, QQnxNativeIo is the
 * Replica side, QIOQnxSourcePrivate is the Source
 * side.  Revisit the name as this matures.
 *
*/
class QQnxNativeIoPrivate;
class QIOQnxSourcePrivate;

class Q_REMOTEOBJECTS_EXPORT QQnxNativeIo : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQnxNativeIo)

public:
    explicit QQnxNativeIo(QObject *parent = nullptr);
    ~QQnxNativeIo();

    bool connectToServer(OpenMode openMode = ReadWrite);
    bool connectToServer(const QString &name, OpenMode openMode = ReadWrite);
    void disconnectFromServer();

    void setServerName(const QString &name);
    QString serverName() const;

    void abort();
    bool isSequential() const override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;
    bool open(OpenMode openMode = ReadWrite) override;
    void close() override;
    QAbstractSocket::SocketError error() const;
    bool flush();
    bool isValid() const;

    QAbstractSocket::SocketState state() const;
    bool waitForBytesWritten(int msecs = 30000) override;
    bool waitForConnected(int msecs = 30000);
    bool waitForDisconnected(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000) override;

Q_SIGNALS:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);
    void stateChanged(QAbstractSocket::SocketState socketState);

protected:
    qint64 readData(char*, qint64) override;
    qint64 writeData(const char*, qint64) override;

private:
    Q_DISABLE_COPY(QQnxNativeIo)
};
Q_DECLARE_TYPEINFO(QQnxNativeIo, Q_MOVABLE_TYPE);

class Q_REMOTEOBJECTS_EXPORT QIOQnxSource : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QIOQnxSource)

public:
    explicit QIOQnxSource(int rcvid, QObject *parent = nullptr);
    ~QIOQnxSource();

    bool isSequential() const override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;
    void close() override;
    QAbstractSocket::SocketError error() const;
    bool isValid() const;

    QAbstractSocket::SocketState state() const;
    bool waitForBytesWritten(int msecs = 30000) override;
    bool waitForConnected(int msecs = 30000);
    bool waitForDisconnected(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000) override;

Q_SIGNALS:
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);
    void stateChanged(QAbstractSocket::SocketState socketState);

protected:
    qint64 readData(char*, qint64) override;
    qint64 writeData(const char*, qint64) override;
    bool open(OpenMode openMode) override;

private Q_SLOTS:
    void onDisconnected();

private:
    Q_DISABLE_COPY(QIOQnxSource)
    friend class QQnxNativeServerPrivate;
    friend class QnxServerIo;
};
Q_DECLARE_TYPEINFO(QIOQnxSource, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // QQNXNATIVEIO_H
