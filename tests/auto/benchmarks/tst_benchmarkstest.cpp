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

#include <QString>
#include <QDataStream>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtTest>
#include <QtRemoteObjects/QRemoteObjectNode>
#include "rep_localdatacenter_replica.h"
#include "rep_localdatacenter_source.h"

class BenchmarksTest : public QObject
{
    Q_OBJECT

public:
    BenchmarksTest();
private:
    QRemoteObjectHost m_basicServer;
    QRemoteObjectNode m_basicClient;
    QScopedPointer<LocalDataCenterSimpleSource> dataCenterLocal;
private Q_SLOTS:
    void initTestCase();
    void benchPropertyChangesInt();
    void benchQDataStreamInt();
    void benchQLocalSocketInt();
    void benchQLocalSocketQDataStreamInt();
};

BenchmarksTest::BenchmarksTest()
{
}

void BenchmarksTest::initTestCase() {
    m_basicServer.setHostUrl(QUrl(QStringLiteral("local:replica")));
    dataCenterLocal.reset(new LocalDataCenterSimpleSource);
    dataCenterLocal->setData1(5);
    const bool remoted = m_basicServer.enableRemoting(dataCenterLocal.data());
    Q_ASSERT(remoted);
    Q_UNUSED(remoted);

    m_basicClient.connectToNode(QUrl(QStringLiteral("local:replica")));
    Q_ASSERT(m_basicClient.lastError() == QRemoteObjectNode::NoError);
}

void BenchmarksTest::benchPropertyChangesInt()
{
    QScopedPointer<LocalDataCenterReplica> center;
    center.reset(m_basicClient.acquire<LocalDataCenterReplica>());
    if (!center->isInitialized()) {
        QEventLoop loop;
        connect(center.data(), &LocalDataCenterReplica::initialized, &loop, &QEventLoop::quit);
        loop.exec();
    }
    QEventLoop loop;
    int lastValue = 0;
    connect(center.data(), &LocalDataCenterReplica::data1Changed, [&lastValue, &center, &loop]() {
        const bool res = (lastValue++ == center->data1());
        Q_ASSERT(res);
        Q_UNUSED(res);
        if (lastValue == 50000)
            loop.quit();
    });
    QBENCHMARK {
        for (int i = 0; i < 50000; ++i) {
            dataCenterLocal->setData1(i);
        }
        loop.exec();
    }
}
// This ONLY tests the optimal case of a non resizing QByteArray
void BenchmarksTest::benchQDataStreamInt()
{
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    QDataStream rStream(&buffer, QIODevice::ReadOnly);
    int readout = 0;
    QBENCHMARK {
        for (int i = 0; i < 50000; ++i) {
            stream << i;
        }
        for (int i = 0; i < 50000; ++i) {
            rStream >> readout;
            Q_ASSERT(i == readout);
        }
    }
}

void BenchmarksTest::benchQLocalSocketInt()
{
    const QString socketName = QStringLiteral("benchLocalSocket");
    QLocalServer server;
    QLocalServer::removeServer(socketName);
    server.listen(socketName);
    QLocalSocket client;
    client.connectToServer(socketName);
    QEventLoop loop;
    QScopedPointer<QLocalSocket> serverSock;
    if (!server.hasPendingConnections()) {
        connect(&server, &QLocalServer::newConnection, &loop, &QEventLoop::quit);
        loop.exec();
    }
    Q_ASSERT(server.hasPendingConnections());
    serverSock.reset(server.nextPendingConnection());
    int lastValue = 0;
    connect(&client, &QLocalSocket::readyRead, [&loop, &lastValue, &client]() {
        int readout = 0;
        while (client.bytesAvailable() && lastValue < 50000) {
            client.read(reinterpret_cast<char*>(&readout), sizeof(int));
            const bool res = (lastValue++ == readout);
            Q_ASSERT(res);
            Q_UNUSED(res);
        }
        if (lastValue >= 50000)
            loop.quit();
    });
    QBENCHMARK {
        for (int i = 0; i < 50000; ++i) {
            const int res = serverSock->write(reinterpret_cast<char*>(&i), sizeof(int));
            Q_ASSERT(res == sizeof(int));
            Q_UNUSED(res);
        }
        loop.exec();
    }
}

void BenchmarksTest::benchQLocalSocketQDataStreamInt()
{
    const QString socketName = QStringLiteral("benchLocalSocket");
    QLocalServer server;
    QLocalServer::removeServer(socketName);
    server.listen(socketName);
    QLocalSocket client;
    client.connectToServer(socketName);
    QDataStream readStream(&client);
    QEventLoop loop;
    QScopedPointer<QLocalSocket> serverSock;
    if (!server.hasPendingConnections()) {
        connect(&server, &QLocalServer::newConnection, &loop, &QEventLoop::quit);
        loop.exec();
    }
    Q_ASSERT(server.hasPendingConnections());
    serverSock.reset(server.nextPendingConnection());
    QDataStream writeStream(serverSock.data());
    int lastValue = 0;
    connect(&client, &QIODevice::readyRead, [&loop, &lastValue, &readStream]() {
        int readout = 0;
        while (readStream.device()->bytesAvailable() && lastValue < 50000) {
            readStream >> readout;
            const bool res = (lastValue++ == readout);
            Q_ASSERT(res);
            Q_UNUSED(res);
        }
        if (lastValue >= 50000)
            loop.quit();
    });
    QBENCHMARK {
        for (int i = 0; i < 50000; ++i) {
            writeStream << i;
        }
        loop.exec();
    }
}

QTEST_MAIN(BenchmarksTest)

#include "tst_benchmarkstest.moc"
