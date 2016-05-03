/****************************************************************************
**
** Copyright (C) 2016 Ford Motor Company
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

#include "rep_MyInterface_replica.h"

#include <QCoreApplication>
#include <QtRemoteObjects/qremoteobjectnode.h>
#include <QtTest/QtTest>

class tst_Client_Process : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testRun()
    {
        QRemoteObjectNode repNode;
        repNode.connectToNode(QUrl(QStringLiteral("tcp://127.0.0.1:65213")));

        QSharedPointer<MyInterfaceReplica> rep(repNode.acquire<MyInterfaceReplica>());
        QVERIFY(rep->waitForSource());

        auto reply = rep->start();
        QVERIFY(reply.waitForFinished());

        // BEGIN: Testing
        QSignalSpy advanceSpy(rep.data(), SIGNAL(advance()));

        QSignalSpy spy(rep.data(), SIGNAL(enum1Changed(MyInterfaceReplica::Enum1)));
        QVERIFY(advanceSpy.wait());

        QCOMPARE(spy.count(), 2);
        // END: Testing

        reply = rep->stop();
        QVERIFY(reply.waitForFinished());

        qDebug() << "Done. Shutting down.";

        // wait for delivery of events
        QTest::qWait(200);
    }
};

QTEST_MAIN(tst_Client_Process)

#include "main.moc"
