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

#include <QtTest/QtTest>
#include <QMetaType>
#include <qremoteobjectreplica.h>
#include <QRemoteObjectNode>
#include "engine.h"
#include "speedometer.h"
#include "rep_engine_replica.h"
#include "rep_speedometer_replica.h"
#include "rep_localdatacenter_source.h"
#include "rep_tcpdatacenter_source.h"
#include "rep_localdatacenter_replica.h"
#include "rep_tcpdatacenter_replica.h"

//DUMMY impl for variant comparison
bool operator<(const QVector<int> &lhs, const QVector<int> &rhs)
{
    return lhs.size() < rhs.size();
}

class tst_Integration: public QObject
{
    Q_OBJECT
    QRemoteObjectNode m_client;
    QRemoteObjectNode m_registryClient;
    QRemoteObjectNode m_basicServer;
    QRemoteObjectNode m_localCentreServer;
    QRemoteObjectNode m_tcpCentreServer;
    QRemoteObjectNode m_registryServer;
    QRemoteObjectNode m_qobjectServer;
    QRemoteObjectNode m_qobjectClient;
private slots:

    void initTestCase() {
        QLoggingCategory::setFilterRules("*.debug=true\n"
                                         "qt.remoteobjects.debug=false\n"
                                         "qt.remoteobjects.warning=false");

        qRegisterMetaType<Temperature>();
        qRegisterMetaTypeStreamOperators<Temperature>();

        //Setup registry
        //Registry needs to be created first until we get the retry mechanism implemented
        m_registryServer = QRemoteObjectNode::createRegistryHostNode();

        m_client = QRemoteObjectNode();
        m_registryClient = QRemoteObjectNode::createNodeConnectedToRegistry();
        //m_client.setObjectName("DirectTestClient");
        //m_registryClient.setObjectName("RegistryTestClient");

        m_basicServer = QRemoteObjectNode::createHostNode(QUrl("tcp://localhost:9999"));

        engine.reset(new Engine);
        speedometer.reset(new Speedometer);
        m_basicServer.enableRemoting(engine.data());
        m_basicServer.enableRemoting(speedometer.data());

        m_client.connect(QUrl("tcp://localhost:9999"));

        //setup servers
        qRegisterMetaType<QVector<int> >();
        QMetaType::registerComparators<QVector<int> >();
        qRegisterMetaTypeStreamOperators<QVector<int> >();
        m_localCentreServer = QRemoteObjectNode::createHostNodeConnectedToRegistry();
        dataCenterLocal.reset(new LocalDataCenterSimpleSource);
        dataCenterLocal->setData1(5);
        dataCenterLocal->setData2(5.0);
        dataCenterLocal->setData3(QStringLiteral("local"));
        dataCenterLocal->setData4(QVector<int>() << 1 << 2 << 3 << 4 << 5);
        m_localCentreServer.enableRemoting(dataCenterLocal.data());

        m_tcpCentreServer = QRemoteObjectNode::createHostNodeConnectedToRegistry(QUrl("tcp://localhost:19999"));
        dataCenterTcp.reset(new TcpDataCenterSimpleSource);
        dataCenterTcp->setData1(5);
        dataCenterTcp->setData2(5.0);
        dataCenterTcp->setData3(QStringLiteral("tcp"));
        dataCenterTcp->setData4(QVector<int>() << 1 << 2 << 3 << 4 << 5);
        m_tcpCentreServer.enableRemoting(dataCenterTcp.data());

        //Setup the client
        //QVERIFY(m_registryClient.connect( QStringLiteral("local:replica")));
    }

    void cleanup()
    {
        // wait for delivery of RemoveObject events to the source
        QTest::qWait(20);
    }

    void registryTest() {
        QSharedPointer<TcpDataCenterReplica> tcpCentre(m_registryClient.acquire<TcpDataCenterReplica>());
        QSharedPointer<LocalDataCenterReplica> localCentre(m_registryClient.acquire<LocalDataCenterReplica>());
        tcpCentre->waitForSource();
        localCentre->waitForSource();
        QCOMPARE(m_registryClient.registry()->sourceLocations(), m_registryServer.registry()->sourceLocations());
        QVERIFY(localCentre->isInitialized());
        QVERIFY(tcpCentre->isInitialized());

        QCOMPARE(tcpCentre->data1(), 5 );
        QCOMPARE(tcpCentre->data2(), 5.0);
        QCOMPARE(tcpCentre->data3(), QStringLiteral("tcp"));
        QCOMPARE(tcpCentre->data4(), QVector<int>() << 1 << 2 << 3 << 4 << 5);

        QCOMPARE(localCentre->data1(), 5);
        QCOMPARE(localCentre->data2(), 5.0);
        QCOMPARE(localCentre->data3(), QStringLiteral("local"));
        QCOMPARE(localCentre->data4(), QVector<int>() << 1 << 2 << 3 << 4 << 5);

    }

    void basicTest() {
        engine->setRpm(1234);

        QSharedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 1234);
    }

    void defaultValueTest() {
        QSharedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->cylinders(), 4);
    }

    void slotTest() {
        engine->setStarted(false);

        QSharedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->started(), false);

        QRemoteObjectPendingReply<bool> reply = engine_r->start();
        QCOMPARE(reply.error(), QRemoteObjectPendingCall::InvalidMessage);
        QVERIFY(reply.waitForFinished());
        QVERIFY(reply.isFinished());
        QCOMPARE(reply.returnValue(), true);
        QCOMPARE(reply.error(), QRemoteObjectPendingCall::NoError);

        QCOMPARE(engine_r->started(), true);
    }

    void slotTestWithWatcher() {
        engine->setStarted(false);

        QSharedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->started(), false);

        QRemoteObjectPendingReply<bool> reply = engine_r->start();
        QCOMPARE(reply.error(), QRemoteObjectPendingCall::InvalidMessage);

        QRemoteObjectPendingCallWatcher watcher(reply);
        QSignalSpy spy(&watcher, SIGNAL(finished(QRemoteObjectPendingCallWatcher *)));
        spy.wait();
        QCOMPARE(spy.count(), 1);

        QVERIFY(reply.isFinished());
        QCOMPARE(reply.returnValue(), true);
        QCOMPARE(engine_r->started(), true);
    }

    void slotTestDynamicReplica() {
        engine->setStarted(false);

        QSharedPointer<QRemoteObjectDynamicReplica> engine_r(m_client.acquire("Engine"));
        Q_ASSERT(engine_r);
        engine_r->waitForSource();

        const QMetaObject *metaObject = engine_r->metaObject();
        const int propIndex = metaObject->indexOfProperty("started");
        QVERIFY(propIndex >= 0);
        const QMetaProperty property = metaObject->property(propIndex);
        bool started = property.read(engine_r.data()).value<bool>();
        QCOMPARE(started, false);

        const int methodIndex = metaObject->indexOfMethod("start()");
        QVERIFY(methodIndex >= 0);
        const QMetaMethod method = metaObject->method(methodIndex);
        QRemoteObjectPendingCall call;
        QVERIFY(method.invoke(engine_r.data(), Q_RETURN_ARG(QRemoteObjectPendingCall, call)));
        QCOMPARE(call.error(), QRemoteObjectPendingCall::InvalidMessage);
        QVERIFY(call.waitForFinished());
        QVERIFY(call.isFinished());
        QCOMPARE(call.returnValue().type(), QVariant::Bool);
        QCOMPARE(call.returnValue().toBool(), true);
        started = property.read(engine_r.data()).value<bool>();
        QCOMPARE(started, true);
    }

    void slotTestInProcess() {
        engine->setStarted(false);

        QSharedPointer<EngineReplica> engine_r(m_basicServer.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->started(), false);

        QRemoteObjectPendingReply<bool> reply = engine_r->start();
        QVERIFY(reply.waitForFinished());
        QVERIFY(reply.isFinished());
        QCOMPARE(reply.returnValue(), true);
        QCOMPARE(reply.error(), QRemoteObjectPendingCall::NoError);

        QCOMPARE(engine_r->started(), true);
    }

    void slotTestWithUnnormalizedSignature()
    {
        QSharedPointer<EngineReplica> engine_r(m_basicServer.acquire<EngineReplica>());
        engine_r->waitForSource();

        engine_r->unnormalizedSignature(0, 0);
    }

    void slotWithParameterTest() {
        engine->setRpm(0);

        QSharedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 0);

        QSignalSpy spy(engine_r.data(), SIGNAL(rpmChanged()));
        engine_r->increaseRpm(1000);
        spy.wait();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(engine_r->rpm(), 1000);
    }

    void slotWithUserReturnTypeTest() {
        engine->setTemperature(Temperature(400, "Kelvin"));

        QSharedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QRemoteObjectPendingReply<Temperature> pendingReply = engine_r->temperature();
        pendingReply.waitForFinished();
        Temperature temperature = pendingReply.returnValue();
        QCOMPARE(temperature, Temperature(400, "Kelvin"));
    }

    void sequentialReplicaTest() {
        engine->setRpm(3456);

        QSharedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 3456);

        engine_r =  QSharedPointer<EngineReplica>(m_client.acquire< EngineReplica >());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 3456);
    }

    void doubleReplicaTest() {
        QSharedPointer<EngineReplica> engine_r1(m_client.acquire< EngineReplica >());
        QSharedPointer<EngineReplica> engine_r2(m_client.acquire< EngineReplica >());
        engine->setRpm(3412);

        engine_r1->waitForSource();
        engine_r2->waitForSource();

        QCOMPARE(engine_r1->rpm(), 3412);
        QCOMPARE(engine_r2->rpm(), 3412);
    }

    void twoReplicaTest() {
        engine->setRpm(1234);
        speedometer->setMph(70);

        QSharedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QSharedPointer<SpeedometerReplica> speedometer_r(m_client.acquire<SpeedometerReplica>());
        speedometer_r->waitForSource();

        QCOMPARE(engine_r->rpm(), 1234);
        QCOMPARE(speedometer_r->mph(), 70);
    }

    void dynamicReplicaTest() {
        QRemoteObjectDynamicReplica *rep1 = m_registryClient.acquire("TcpDataCenter");
        QRemoteObjectDynamicReplica *rep2 = m_registryClient.acquire("TcpDataCenter");
        QRemoteObjectDynamicReplica *rep3 = m_registryClient.acquire("LocalDataCenter");
        rep1->waitForSource();
        rep2->waitForSource();
        rep3->waitForSource();
        const QMetaObject *metaTcpRep1 = rep1->metaObject();
        const QMetaObject *metaLocalRep1 = rep3->metaObject();
        const QMetaObject *metaTcpSource = dataCenterTcp->metaObject();
        const QMetaObject *metaLocalSource = dataCenterLocal->metaObject();
        QVERIFY(rep1->isInitialized());
        QVERIFY(rep2->isInitialized());
        QVERIFY(rep3->isInitialized());

        for (int i = 0; i < metaTcpRep1->propertyCount(); ++i)
        {
            const QMetaProperty propLhs =  metaTcpRep1->property(i);
            if (qstrcmp(propLhs.name(), "isReplicaValid") == 0) //Ignore properties only on the Replica side
                continue;
            const QMetaProperty propRhs =  metaTcpSource->property(metaTcpSource->indexOfProperty(propLhs.name()));
            if (propLhs.notifySignalIndex() == -1)
                QCOMPARE(propRhs.hasNotifySignal(), false);
            else {
                QCOMPARE(propRhs.notifySignalIndex() != -1, true);
                QCOMPARE(metaTcpRep1->method(propLhs.notifySignalIndex()).name(), metaTcpSource->method(propRhs.notifySignalIndex()).name());
            }
            QCOMPARE(propLhs.read(rep1),  propRhs.read(dataCenterTcp.data()));
        }
        for (int i = 0; i < metaLocalRep1->propertyCount(); ++i )
        {
            const QMetaProperty propLhs =  metaLocalRep1->property(i);
            if (qstrcmp(propLhs.name(), "isReplicaValid") == 0) //Ignore properties only on the Replica side
                continue;
            const QMetaProperty propRhs =  metaLocalSource->property(metaTcpSource->indexOfProperty(propLhs.name()));
            if (propLhs.notifySignalIndex() == -1)
                QCOMPARE(propRhs.hasNotifySignal(), false);
            else {
                QCOMPARE(propRhs.notifySignalIndex() != -1, true);
                QCOMPARE(metaTcpRep1->method(propLhs.notifySignalIndex()).name(), metaTcpSource->method(propRhs.notifySignalIndex()).name());
            }
            QCOMPARE(propLhs.read(rep3),  propRhs.read(dataCenterLocal.data()));
        }

    }

    void apiTest() {
        m_qobjectServer = QRemoteObjectNode::createHostNode(QUrl("local:qobject"));

        m_qobjectServer.enableRemoting<EngineSourceAPI>(engine.data());

        engine->setRpm(1234);

        m_qobjectClient = QRemoteObjectNode();
        m_qobjectClient.connect(QUrl("local:qobject"));
        QSharedPointer<EngineReplica> engine_r(m_qobjectClient.acquire<EngineReplica>());
        engine_r->waitForSource();

        QCOMPARE(engine_r->rpm(), 1234);
    }

    void apiInProcTest() {
        QSharedPointer<EngineReplica> engine_r_inProc(m_qobjectServer.acquire<EngineReplica>());
        engine_r_inProc->waitForSource();

        QCOMPARE(engine_r_inProc->rpm(), 1234);
    }


private:
    QScopedPointer<Engine> engine;
    QScopedPointer<Speedometer> speedometer;
    QScopedPointer<TcpDataCenterSimpleSource> dataCenterTcp;
    QScopedPointer<LocalDataCenterSimpleSource> dataCenterLocal;
};

QTEST_MAIN(tst_Integration)

#include "tst_integration.moc"
