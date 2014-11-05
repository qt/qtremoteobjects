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
private slots:

    void initTestCase() {
        QLoggingCategory::setFilterRules("*.debug=true\n"
                                         "qt.remoteobjects.debug=false\n"
                                         "qt.remoteobjects.warning=false");
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
        dataCenterLocal.reset(new LocalDataCenterSource);
        dataCenterLocal->setData1(5);
        dataCenterLocal->setData2(5.0);
        dataCenterLocal->setData3(QStringLiteral("local"));
        dataCenterLocal->setData4(QVector<int>() << 1 << 2 << 3 << 4 << 5);
        m_localCentreServer.enableRemoting(dataCenterLocal.data());

        m_tcpCentreServer = QRemoteObjectNode::createHostNodeConnectedToRegistry(QUrl("tcp://localhost:19999"));
        dataCenterTcp.reset(new TcpDataCenterSource);
        dataCenterTcp->setData1(5);
        dataCenterTcp->setData2(5.0);
        dataCenterTcp->setData3(QStringLiteral("tcp"));
        dataCenterTcp->setData4(QVector<int>() << 1 << 2 << 3 << 4 << 5);
        m_tcpCentreServer.enableRemoting(dataCenterTcp.data());

        //Setup the client
        //QVERIFY(m_registryClient.connect( QStringLiteral("local:replica")));
    }

    void RegistryTest() {
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
            const QMetaProperty propRhs =  metaTcpSource->property(metaTcpSource->indexOfProperty(propLhs.name()));
            QCOMPARE(propLhs.notifySignalIndex(),  propRhs.notifySignalIndex());
            QCOMPARE(propLhs.read(rep1),  propRhs.read(dataCenterTcp.data()));
            QCOMPARE(propLhs.read(rep2),  propRhs.read(rep1));
        }
        for (int i = 0; i < metaLocalRep1->propertyCount(); ++i )
        {
            const QMetaProperty propLhs =  metaLocalRep1->property(i);
            const QMetaProperty propRhs =  metaLocalSource->property(metaTcpSource->indexOfProperty(propLhs.name()));
            QCOMPARE(propLhs.notifySignalIndex(),  propRhs.notifySignalIndex());
            QCOMPARE(propLhs.read(rep3),  propRhs.read(dataCenterLocal.data()));
        }

    }

private:
    QScopedPointer<Engine> engine;
    QScopedPointer<Speedometer> speedometer;
    QScopedPointer<TcpDataCenterSource> dataCenterTcp;
    QScopedPointer<LocalDataCenterSource> dataCenterLocal;
};

QTEST_MAIN(tst_Integration)

#include "tst_integration.moc"
