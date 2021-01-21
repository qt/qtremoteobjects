/****************************************************************************
**
** Copyright (C) 2017-2016 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQNXNATIVESERVER_H
#define QQNXNATIVESERVER_H

#include <QtNetwork/qabstractsocket.h>
#include <QtRemoteObjects/qtremoteobjectglobal.h>

QT_BEGIN_NAMESPACE

class QQnxNativeServerPrivate;
class QQnxNativeIo;
class QIOQnxSource;

class Q_REMOTEOBJECTS_EXPORT QQnxNativeServer : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQnxNativeServer)

Q_SIGNALS:
    void newConnection();

public:
    explicit QQnxNativeServer(QObject *parent = nullptr);
    ~QQnxNativeServer();

    void close();
    bool hasPendingConnections() const;
    bool isListening() const;
    bool listen(const QString &name);
    QSharedPointer<QIOQnxSource> nextPendingConnection();
    QString serverName() const;
    bool waitForNewConnection(int msec = 0, bool *timedOut = nullptr);

private Q_SLOTS:
    void onSourceClosed();

private:
    Q_DISABLE_COPY(QQnxNativeServer)
    friend class QIOQnxSource;
};

QT_END_NAMESPACE

#endif // QQNXNATIVESERVER_H
