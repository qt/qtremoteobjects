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

#include "mytestserver.h"

#include <QCoreApplication>
#include <QtTest/QtTest>

class tst_Server_Process : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testRun()
    {
        QRemoteObjectHost srcNode(QUrl(QStringLiteral("tcp://127.0.0.1:65213")));
        MyTestServer myTestServer;

        srcNode.enableRemoting(&myTestServer);

        qDebug() << "Waiting for incoming connections";

        QSignalSpy waitForStartedSpy(&myTestServer, SIGNAL(startedChanged(bool)));
        QVERIFY(waitForStartedSpy.isValid());
        QVERIFY(waitForStartedSpy.wait());
        QCOMPARE(waitForStartedSpy.value(0).value(0).toBool(), true);

        // wait for delivery of events
        QTest::qWait(200);

        qDebug() << "Client connected";

        // BEGIN: Testing

        // make sure continuous changes to enums don't mess up the protocol
        myTestServer.setEnum1(MyTestServer::Second);
        myTestServer.setEnum1(MyTestServer::Third);

        emit myTestServer.advance();

        // END: Testing

        waitForStartedSpy.clear();
        QVERIFY(waitForStartedSpy.wait());
        QCOMPARE(waitForStartedSpy.value(0).value(0).toBool(), false);

        qDebug() << "Done. Shutting down.";

        // wait for delivery of events
        QTest::qWait(200);
    }
};

QTEST_MAIN(tst_Server_Process)

#include "main.moc"
