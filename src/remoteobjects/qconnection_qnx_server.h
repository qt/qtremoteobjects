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
    explicit QQnxNativeServer(QObject *parent = Q_NULLPTR);
    ~QQnxNativeServer();

    void close();
    bool hasPendingConnections() const;
    bool isListening() const;
    bool listen(const QString &name);
    QIOQnxSource *nextPendingConnection();
    QString serverName() const;
    bool waitForNewConnection(int msec = 0, bool *timedOut = Q_NULLPTR);

private Q_SLOTS:
    void onSourceClosed();

private:
    Q_DISABLE_COPY(QQnxNativeServer)
    friend class QIOQnxSource;
};

QT_END_NAMESPACE

#endif // QQNXNATIVESERVER_H
