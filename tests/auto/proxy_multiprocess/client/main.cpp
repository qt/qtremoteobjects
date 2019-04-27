/****************************************************************************
**
** Copyright (C) 2019 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rep_subclass_replica.h"
#include "../shared.h"

#include <QCoreApplication>
#include <QtRemoteObjects/qremoteobjectnode.h>
#include <QtTest/QtTest>

class tst_Client_Process : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_repNode.reset(new QRemoteObjectNode);
        m_repNode->connectToNode(QUrl(QStringLiteral("tcp://127.0.0.1:65213")));
        m_rep.reset(m_repNode->acquire<ParentClassReplica>());
        const auto objectMode = qEnvironmentVariable("ObjectMode");
        qDebug() << "Waiting to connect, mode =" << objectMode;
        QVERIFY(m_rep->waitForSource());
    }

    void testSubClass()
    {
        const auto objectMode = qEnvironmentVariable("ObjectMode");

        qDebug() << "Starting test" << objectMode;
        if (objectMode == QLatin1Literal("ObjectPointer")) {
            QSignalSpy tracksSpy(m_rep->tracks(), &QAbstractItemModelReplica::initialized);
            QVERIFY(m_rep->subClass() != nullptr);
            QCOMPARE(m_rep->subClass()->myPOD(), initialValue);
            QCOMPARE(m_rep->subClass()->i(), initialI);
            QVERIFY(m_rep->tracks() != nullptr);
            QVERIFY(tracksSpy.count() || tracksSpy.wait());
            QCOMPARE(m_rep->variant(), QVariant::fromValue(42.0f));
        } else {
            QVERIFY(m_rep->subClass() == nullptr);
            QVERIFY(m_rep->tracks() == nullptr);
            QCOMPARE(m_rep->variant(), QVariant());
        }

        qDebug() << "Verified expected initial states, sending start.";
        auto reply = m_rep->start();
        QVERIFY(reply.waitForFinished());

        QSignalSpy advanceSpy(m_rep.data(), SIGNAL(advance()));
        QVERIFY(advanceSpy.wait());
        QVERIFY(m_rep->subClass() != nullptr);
        QCOMPARE(m_rep->subClass()->myPOD(), updatedValue);
        QCOMPARE(m_rep->subClass()->i(), updatedI);
        QVERIFY(m_rep->tracks() != nullptr);
        QCOMPARE(m_rep->variant(), QVariant::fromValue(podValue));
        qDebug() << "Verified expected final states, cleaning up.";
    }

    void cleanupTestCase()
    {
        auto reply = m_rep->quit();
        QVERIFY(reply.waitForFinished());
    }

private:
    QScopedPointer<QRemoteObjectNode> m_repNode;
    QScopedPointer<ParentClassReplica> m_rep;
};

QTEST_MAIN(tst_Client_Process)

#include "main.moc"
