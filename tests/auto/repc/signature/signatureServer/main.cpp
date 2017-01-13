/****************************************************************************
**
** Copyright (C) 2016 Ford Motor Company
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

#include "rep_server_source.h"

#include <QCoreApplication>
#include <QtTest/QtTest>

class MyTestServer : public TestClassSimpleSource
{
    Q_OBJECT
public:
    bool shouldQuit = false;
    // TestClassSimpleSource interface
public slots:
    bool slot1() Q_DECL_OVERRIDE {return true;}
    QString slot2() Q_DECL_OVERRIDE {return QLatin1String("Hello there");}
    void ping(const QString &message) Q_DECL_OVERRIDE
    {
        emit pong(message);
    }

    bool quit() Q_DECL_OVERRIDE
    {
        qDebug() << "quit() called";

        return shouldQuit = true;
    }
};

class tst_Server_Process : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testRun()
    {
        QRemoteObjectHost srcNode(QUrl(QStringLiteral("tcp://127.0.0.1:65214")));
        MyTestServer myTestServer{};

        srcNode.enableRemoting(&myTestServer);

        qDebug() << "Waiting for incoming connections";

        QTRY_VERIFY_WITH_TIMEOUT(myTestServer.shouldQuit, 20000); // wait up to 20s

        qDebug() << "Stopping server";
        QTest::qWait(200); // wait for server to send reply to client invoking quit() function
    }
};

QTEST_MAIN(tst_Server_Process)

#include "main.moc"
