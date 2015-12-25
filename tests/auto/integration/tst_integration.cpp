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
#include <QProcess>
#include <QFileInfo>
#include <qremoteobjectreplica.h>
#include <QRemoteObjectNode>
#include "engine.h"
#include "speedometer.h"
#include "rep_engine_replica.h"
#include "rep_speedometer_merged.h"
#include "rep_enum_merged.h"
#include "rep_pod_merged.h"
#include "rep_localdatacenter_source.h"
#include "rep_tcpdatacenter_source.h"
#include "rep_localdatacenter_replica.h"
#include "rep_tcpdatacenter_replica.h"

#define SET_NODE_NAME(obj) (obj).setName(QLatin1String(#obj))

//DUMMY impl for variant comparison
bool operator<(const QVector<int> &lhs, const QVector<int> &rhs)
{
    return lhs.size() < rhs.size();
}

class TestLargeData: public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void send(const QByteArray &data);
};

class tst_Integration: public QObject
{
    Q_OBJECT
    QRemoteObjectNode m_client;
    QRemoteObjectNode m_registryClient;
    QRemoteObjectHost m_basicServer;
    QRemoteObjectHost m_localCentreServer;
    QRemoteObjectHost m_tcpCentreServer;
    QRemoteObjectRegistryHost m_registryServer;
    QRemoteObjectHost m_qobjectServer;
    QRemoteObjectNode m_qobjectClient;
    QScopedPointer<EngineReplica> m_regBase;
    QScopedPointer<EngineReplica> m_regNamed;
    QScopedPointer<QRemoteObjectDynamicReplica> m_regDynamic;
    QScopedPointer<QRemoteObjectDynamicReplica> m_regDynamicNamed;
    int m_regAdded;

signals:
    void forwardResult(int);

public slots:
    void onAdded(QRemoteObjectSourceLocation entry)
    {
        if (entry.first == QLatin1String("Engine")) {
            ++m_regAdded;
            //Add regular replica first, then dynamic one
            m_regBase.reset(m_registryClient.acquire<EngineReplica>());
            m_regDynamic.reset(m_registryClient.acquire(QStringLiteral("Engine")));
        }
        if (entry.first == QLatin1String("MyTestEngine")) {
            m_regAdded += 2;
            //Now add dynamic replica first, then regular one
            m_regDynamicNamed.reset(m_registryClient.acquire(QStringLiteral("MyTestEngine")));
            m_regNamed.reset(m_registryClient.acquire<EngineReplica>(QStringLiteral("MyTestEngine")));
        }
    }

    void onInitialized() {
        QObject *obj = sender();
        QVERIFY(obj);
        const QMetaObject *metaObject = obj->metaObject();
        const int propIndex = metaObject->indexOfProperty("rpm");
        QVERIFY(propIndex >= 0);
        const QMetaProperty mp =  metaObject->property(propIndex);
        QVERIFY(connect(obj, QByteArray(QByteArrayLiteral("2")+mp.notifySignal().methodSignature().constData()), this, SIGNAL(forwardResult(int))));
    }

private slots:

    void initTestCase() {
        QLoggingCategory::setFilterRules(QLatin1String(
                                         "*.debug=true\n"
                                         "qt.remoteobjects.debug=false\n"
                                         "qt.remoteobjects.warning=false"));

        //Setup registry
        //Registry needs to be created first until we get the retry mechanism implemented
        m_registryServer.setHostUrl(QUrl(QStringLiteral("local:registry")));
        SET_NODE_NAME(m_registryServer);

        SET_NODE_NAME(m_client);
        m_registryClient.setRegistryUrl(QUrl(QStringLiteral("local:registry")));
        SET_NODE_NAME(m_registryClient);
        const bool res = m_registryClient.waitForRegistry(3000);
        QVERIFY(res);

        m_basicServer.setHostUrl(QUrl(QStringLiteral("tcp://127.0.0.1:0")));
        SET_NODE_NAME(m_basicServer);

        engine.reset(new Engine);
        speedometer.reset(new Speedometer);
        m_basicServer.enableRemoting(engine.data());
        m_basicServer.enableRemoting(speedometer.data());

        m_client.connectToNode(m_basicServer.hostUrl());

        //setup servers
        QMetaType::registerComparators<QVector<int> >();
        m_localCentreServer.setHostUrl(QUrl(QStringLiteral("local:local")));
        m_localCentreServer.setRegistryUrl(QUrl(QStringLiteral("local:registry")));
        SET_NODE_NAME(m_localCentreServer);
        dataCenterLocal.reset(new LocalDataCenterSimpleSource);
        dataCenterLocal->setData1(5);
        dataCenterLocal->setData2(5.0);
        dataCenterLocal->setData3(QStringLiteral("local"));
        dataCenterLocal->setData4(QVector<int>() << 1 << 2 << 3 << 4 << 5);
        m_localCentreServer.enableRemoting(dataCenterLocal.data());

        m_tcpCentreServer.setHostUrl(QUrl(QStringLiteral("tcp://127.0.0.1:0")));
        m_tcpCentreServer.setRegistryUrl(QUrl(QStringLiteral("local:registry")));
        SET_NODE_NAME(m_tcpCentreServer);
        dataCenterTcp.reset(new TcpDataCenterSimpleSource);
        dataCenterTcp->setData1(5);
        dataCenterTcp->setData2(5.0);
        dataCenterTcp->setData3(QStringLiteral("tcp"));
        dataCenterTcp->setData4(QVector<int>() << 1 << 2 << 3 << 4 << 5);
        m_tcpCentreServer.enableRemoting(dataCenterTcp.data());

        //Setup the client
        //QVERIFY(m_registryClient.connectToNode( QStringLiteral("local:replica")));
    }

    void cleanup()
    {
        // wait for delivery of RemoveObject events to the source
        QTest::qWait(200);
    }

    void enumTest() {
        TestClassSimpleSource tc;
        tc.setTestEnum(TestEnum::FALSE);
        tc.setClassEnum(TestClassSimpleSource::One);
        m_basicServer.enableRemoting(&tc);
        const QScopedPointer<TestClassReplica> tc_rep(m_client.acquire<TestClassReplica>());
        tc_rep->waitForSource();
        QCOMPARE(tc.testEnum(), tc_rep->testEnum());
        QCOMPARE((qint32)tc.classEnum(), (qint32)TestClassSimpleSource::One);

        // set property on the replica (test property change packet)
        {
            QSignalSpy spy(tc_rep.data(), SIGNAL(classEnumChanged(TestClassReplica::ClassEnum)));
            tc_rep->setClassEnum(TestClassReplica::Two);
            QVERIFY(spy.wait());

            QCOMPARE((qint32)tc.classEnum(), (qint32)tc_rep->classEnum());
        }

        // set property on the source (test property change packet)
        {
            QSignalSpy spy(tc_rep.data(), SIGNAL(classEnumChanged(TestClassReplica::ClassEnum)));
            tc.setClassEnum(TestClassSimpleSource::One);
            QVERIFY(spy.wait());

            QCOMPARE((qint32)tc.classEnum(), (qint32)tc_rep->classEnum());
        }

        QScopedPointer<QRemoteObjectDynamicReplica> tc_repDynamic(m_client.acquire(QStringLiteral("TestClass")));

        tc_repDynamic->waitForSource(1000);
        QVERIFY(tc_repDynamic->isInitialized());

        const QMetaObject *metaObject = tc_repDynamic->metaObject();

        const int propertyIndex = metaObject->indexOfProperty("classEnum");
        QVERIFY(propertyIndex >= 0);

        const QMetaProperty property = metaObject->property(propertyIndex);
        QVERIFY(property.isValid());
        QCOMPARE(property.typeName(), "ClassEnum");

        // read enum on the dynamic replica
        {
            QCOMPARE(property.read(tc_repDynamic.data()).value<TestClassReplica::ClassEnum>(), TestClassReplica::One);
        }

        // write enum on the dynamic replica
        {
            QSignalSpy spy(tc_rep.data(), SIGNAL(classEnumChanged(TestClassReplica::ClassEnum)));
            property.write(tc_repDynamic.data(), TestClassReplica::Two);
            QVERIFY(spy.wait());

            QCOMPARE(tc_rep->classEnum(), TestClassReplica::Two);
        }
    }

    void namedObjectTest() {
        engine->setRpm(3333);
        Engine *e = new Engine();
        QScopedPointer<Engine> engineSave;
        engineSave.reset(e);
        e->setRpm(4444);
        m_basicServer.enableRemoting(e, QStringLiteral("MyTestEngine"));

        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        const QScopedPointer<EngineReplica> namedEngine_r(m_client.acquire<EngineReplica>(QStringLiteral("MyTestEngine")));
        engine_r->waitForSource();
        QCOMPARE(engine_r->cylinders(), 4);
        QCOMPARE(engine_r->rpm(), 3333);
        namedEngine_r->waitForSource();
        QCOMPARE(namedEngine_r->cylinders(), 4);
        QCOMPARE(namedEngine_r->rpm(), 4444);

        engineSave.reset();
        //Deleting the object before disable remoting will cause disable remoting to
        //return false;
        QVERIFY(!m_basicServer.disableRemoting(e));
    }

    void registryAddedTest() {
        m_regAdded = 0;
        connect(m_registryClient.registry(), &QRemoteObjectRegistry::remoteObjectAdded, this, &tst_Integration::onAdded);

        QSignalSpy addedSpy(m_registryClient.registry(), SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)));

        Engine e;
        e.setRpm(1111);
        m_localCentreServer.enableRemoting(&e);
        Engine e2;
        e2.setRpm(2222);
        m_localCentreServer.enableRemoting(&e2, QStringLiteral("MyTestEngine"));
        while (m_regAdded < 3) {
            QVERIFY(addedSpy.wait());
        }
        m_regBase->waitForSource(1000);
        m_regNamed->waitForSource(1000);
        m_regDynamic->waitForSource(1000);
        m_regDynamicNamed->waitForSource(1000);
        QVERIFY(m_regBase->isInitialized());
        QCOMPARE(m_regBase->rpm(),1111);
        QVERIFY(m_regNamed->isInitialized());
        QCOMPARE(m_regNamed->rpm(),2222);

        QVERIFY(m_regDynamic->isInitialized());
        const QMetaObject *metaObject = m_regDynamic->metaObject();

        const int propertyIndex = metaObject->indexOfProperty("rpm");
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = metaObject->property(propertyIndex);
        QVERIFY(property.isValid());

        QCOMPARE(property.read(m_regDynamic.data()).toInt(),e.rpm());

        QVERIFY(m_regDynamicNamed->isInitialized());
        QCOMPARE(property.read(m_regDynamicNamed.data()).toInt(),e2.rpm());

        QVERIFY(m_localCentreServer.disableRemoting(&e));
        QVERIFY(m_localCentreServer.disableRemoting(&e2));
    }

    void registryTest() {
        const QScopedPointer<TcpDataCenterReplica> tcpCentre(m_registryClient.acquire<TcpDataCenterReplica>());
        const QScopedPointer<LocalDataCenterReplica> localCentre(m_registryClient.acquire<LocalDataCenterReplica>());
        QTRY_VERIFY(tcpCentre->waitForSource(100));
        QTRY_VERIFY(localCentre->waitForSource(100));
        //TODO this still fails intermittantly.  Fix that in another patch, though.
        //qDebug() << m_registryClient.registry()->sourceLocations() << m_registryServer.registry()->sourceLocations();
        //QCOMPARE(m_registryClient.registry()->sourceLocations(), m_registryServer.registry()->sourceLocations());
        QTRY_VERIFY(localCentre->isInitialized());
        QTRY_VERIFY(tcpCentre->isInitialized());

        QCOMPARE(tcpCentre->data1(), 5 );
        QCOMPARE(tcpCentre->data2(), 5.0);
        QCOMPARE(tcpCentre->data3(), QStringLiteral("tcp"));
        QCOMPARE(tcpCentre->data4(), QVector<int>() << 1 << 2 << 3 << 4 << 5);

        QCOMPARE(localCentre->data1(), 5);
        QCOMPARE(localCentre->data2(), 5.0);
        QCOMPARE(localCentre->data3(), QStringLiteral("local"));
        QCOMPARE(localCentre->data4(), QVector<int>() << 1 << 2 << 3 << 4 << 5);

    }

    void noRegistryTest() {
        QRemoteObjectHost regReplica(QUrl(QStringLiteral("local:testHost")), QUrl(QStringLiteral("local:testRegistry")));
        SET_NODE_NAME(regReplica);
        const bool res = regReplica.waitForRegistry(3000);
        QVERIFY(!res);
        QCOMPARE(regReplica.registry()->isInitialized(), false);
        const QScopedPointer<Engine> localEngine(new Engine);
        regReplica.enableRemoting(localEngine.data());
        QCOMPARE(regReplica.registry()->sourceLocations().keys().isEmpty(), true);
    }

    void delayedRegistryTest() {
        QRemoteObjectHost regReplica(QUrl(QStringLiteral("local:testHost")),QUrl(QStringLiteral("local:testRegistry")));
        SET_NODE_NAME(regReplica);
        const bool res = regReplica.waitForRegistry(3000);
        QVERIFY(!res);
        QCOMPARE(regReplica.registry()->isInitialized(), false);
        const QScopedPointer<Engine> localEngine(new Engine);
        regReplica.enableRemoting(localEngine.data());
        QCOMPARE(regReplica.registry()->sourceLocations().keys().isEmpty(), true);
        QSignalSpy spy(regReplica.registry(), SIGNAL(initialized()));
        QSignalSpy addedSpy(regReplica.registry(), SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)));
        QRemoteObjectRegistryHost regSource(QUrl(QStringLiteral("local:testRegistry")));
        SET_NODE_NAME(regSource);
        bool added = addedSpy.wait();
        QVERIFY(spy.count() > 0);
        QCOMPARE(added, true);
        QCOMPARE(regReplica.registry()->sourceLocations().keys().isEmpty(), false);
        QCOMPARE(regReplica.registry()->sourceLocations().keys().at(0), QStringLiteral("Engine"));
        QCOMPARE(regReplica.registry()->sourceLocations().value(QStringLiteral("Engine")), QUrl(QStringLiteral("local:testHost")));
        //This should produce a warning...
        regSource.enableRemoting(localEngine.data());
        QVERIFY(regReplica.registry()->sourceLocations().value(QStringLiteral("Engine")) != QUrl(QStringLiteral("local:testRegistry")));

    }

    void basicTest() {
        engine->setRpm(1234);

        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 1234);
    }

    void defaultValueTest() {
        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->cylinders(), 4);
    }

    void notifyTest() {
        engine->setRpm(2345);

        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        QSignalSpy spy(engine_r.data(), SIGNAL(rpmChanged(int)));
        spy.wait();
        QCOMPARE(spy.count(), 1);
        const QList<QVariant> &arguments = spy.first();
        bool ok;
        int res = arguments.at(0).toInt(&ok);
        QVERIFY(ok);
        QCOMPARE(res, engine->rpm());
        QCOMPARE(engine_r->rpm(), engine->rpm());
    }

    void dynamicNotifyTest() {
        engine->setRpm(3456);
        QSignalSpy spy(this, SIGNAL(forwardResult(int)));
        QScopedPointer<QRemoteObjectDynamicReplica> engine_dr(m_client.acquire(QStringLiteral("Engine")));
        connect(engine_dr.data(), &QRemoteObjectDynamicReplica::initialized, this, &tst_Integration::onInitialized);
        spy.wait();
        QCOMPARE(spy.count(), 1);
        const QList<QVariant> &arguments = spy.first();
        bool ok;
        int res = arguments.at(0).toInt(&ok);
        QVERIFY(ok);
        QCOMPARE(res, engine->rpm());
    }

    void slotTest() {
        engine->setStarted(false);

        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        QEventLoop loop;
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        connect(engine_r.data(), &EngineReplica::initialized, &loop, &QEventLoop::quit);
        if (!engine_r->isInitialized())
            loop.exec();
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

        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        QEventLoop loop;
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        connect(engine_r.data(), &EngineReplica::initialized, &loop, &QEventLoop::quit);
        if (!engine_r->isInitialized())
            loop.exec();
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

        const QScopedPointer<QRemoteObjectDynamicReplica> engine_r(m_client.acquire(QStringLiteral("Engine")));
        Q_ASSERT(engine_r);
        QEventLoop loop;
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        connect(engine_r.data(), &EngineReplica::initialized, &loop, &QEventLoop::quit);
        if (!engine_r->isInitialized())
            loop.exec();

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

    void slotTestDynamicReplicaWithArguments() {
        const QScopedPointer<QRemoteObjectDynamicReplica> engine_r(m_client.acquire(QStringLiteral("Engine")));
        Q_ASSERT(engine_r);
        bool ok = engine_r->waitForSource();
        QVERIFY(ok);
        const QMetaObject *metaObject = engine_r->metaObject();

        int methodIndex = metaObject->indexOfMethod("setMyTestString(QString)");
        QVERIFY(methodIndex >= 0);
        QMetaMethod method = metaObject->method(methodIndex);
        QVERIFY(method.isValid());

        // The slot has no return-value, calling it with a Q_RETURN_ARG should fail.
        QRemoteObjectPendingCall setCall;
        QString s = QLatin1String("Hello World 1");
        QVERIFY(!method.invoke(engine_r.data(), Q_RETURN_ARG(QRemoteObjectPendingCall, setCall), Q_ARG(QString, s)));
        QVERIFY(!setCall.waitForFinished());
        QVERIFY(!setCall.isFinished());
        QCOMPARE(setCall.error(), QRemoteObjectPendingCall::InvalidMessage);

        // Now call the method without return-value, that should succeed.
        s = QLatin1String("Hello World 2");
        QVERIFY(method.invoke(engine_r.data(), Q_ARG(QString, s)));

        // Verify that the passed argument was proper set.
        methodIndex = metaObject->indexOfMethod("myTestString()");
        QVERIFY(methodIndex >= 0);
        method = metaObject->method(methodIndex);
        QRemoteObjectPendingCall getCall;
        QVERIFY(method.invoke(engine_r.data(), Q_RETURN_ARG(QRemoteObjectPendingCall, getCall)));
        QVERIFY(getCall.waitForFinished());
        QVERIFY(getCall.isFinished());
        QCOMPARE(getCall.error(), QRemoteObjectPendingCall::NoError);
        QCOMPARE(getCall.returnValue().type(), QVariant::String);
        QCOMPARE(getCall.returnValue().toString(), s);
    }

    void expapiTestDynamicReplica(){
        engine->setStarted(false);
        const QScopedPointer<QRemoteObjectDynamicReplica> engine_r(m_client.acquire(QStringLiteral("Engine")));
        const QMetaObject *metaObject = engine_r->metaObject();
        const int propIndex = metaObject->indexOfProperty("purchasedPart");
        QVERIFY(propIndex < 0);
        const int methodIndex = metaObject->indexOfMethod("setpurchasedPart(bool)");
        QVERIFY(methodIndex < 0);

    }

    void slotTestInProcess() {
        engine->setStarted(false);

        const QScopedPointer<EngineReplica> engine_r(m_basicServer.acquire<EngineReplica>());
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
        const QScopedPointer<EngineReplica> engine_r(m_basicServer.acquire<EngineReplica>());
        engine_r->waitForSource();

        engine_r->unnormalizedSignature(0, 0);
    }

    void setterTest()
    {
        const QScopedPointer<EngineReplica> engine_r(m_basicServer.acquire<EngineReplica>());
        engine_r->waitForSource();
        QSignalSpy spy(engine_r.data(), SIGNAL(rpmChanged(int)));
        engine_r->setRpm(42);
        spy.wait();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(engine_r->rpm(), 42);
    }

    void dynamicSetterTest()
    {
        const QScopedPointer<QRemoteObjectDynamicReplica> engine_dr(m_client.acquire(QStringLiteral("Engine")));
        engine_dr->waitForSource();
        const QMetaObject *metaObject = engine_dr->metaObject();
        const int propIndex = metaObject->indexOfProperty("rpm");
        const QMetaProperty mp =  metaObject->property(propIndex);
        QSignalSpy spy(engine_dr.data(), QByteArray(QByteArrayLiteral("2")+mp.notifySignal().methodSignature().constData()));
        mp.write(engine_dr.data(), 44);
        spy.wait();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(mp.read(engine_dr.data()).toInt(), 44);
    }

    void slotWithParameterTest() {
        engine->setRpm(0);

        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 0);

        QSignalSpy spy(engine_r.data(), SIGNAL(rpmChanged(int)));
        engine_r->increaseRpm(1000);
        spy.wait();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(engine_r->rpm(), 1000);
    }

    void slotWithUserReturnTypeTest() {
        engine->setTemperature(Temperature(400, QStringLiteral("Kelvin")));

        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QRemoteObjectPendingReply<Temperature> pendingReply = engine_r->temperature();
        pendingReply.waitForFinished();
        Temperature temperature = pendingReply.returnValue();
        QCOMPARE(temperature, Temperature(400, QStringLiteral("Kelvin")));
    }

    void sequentialReplicaTest() {
        engine->setRpm(3456);

        QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 3456);

        engine_r.reset(m_client.acquire< EngineReplica >());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 3456);
    }

    void doubleReplicaTest() {
        const QScopedPointer<EngineReplica> engine_r1(m_client.acquire< EngineReplica >());
        const QScopedPointer<EngineReplica> engine_r2(m_client.acquire< EngineReplica >());
        engine->setRpm(3412);

        engine_r1->waitForSource();
        engine_r2->waitForSource();

        QCOMPARE(engine_r1->rpm(), 3412);
        QCOMPARE(engine_r2->rpm(), 3412);
    }

    void twoReplicaTest() {
        engine->setRpm(1234);
        speedometer->setMph(70);

        const QScopedPointer<EngineReplica> engine_r(m_client.acquire<EngineReplica>());
        engine_r->waitForSource();
        const QScopedPointer<SpeedometerReplica> speedometer_r(m_client.acquire<SpeedometerReplica>());
        speedometer_r->waitForSource();

        QCOMPARE(engine_r->rpm(), 1234);
        QCOMPARE(speedometer_r->mph(), 70);
    }

    void dynamicReplicaTest() {
        const QScopedPointer<QRemoteObjectDynamicReplica> rep1(m_registryClient.acquire(QStringLiteral("TcpDataCenter")));
        const QScopedPointer<QRemoteObjectDynamicReplica> rep2(m_registryClient.acquire(QStringLiteral("TcpDataCenter")));
        const QScopedPointer<QRemoteObjectDynamicReplica> rep3(m_registryClient.acquire(QStringLiteral("LocalDataCenter")));
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
            QCOMPARE(propLhs.read(rep1.data()),  propRhs.read(dataCenterTcp.data()));
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
            QCOMPARE(propLhs.read(rep3.data()),  propRhs.read(dataCenterLocal.data()));
        }

    }

    void apiTest() {
        m_qobjectServer.setHostUrl(QUrl(QStringLiteral("local:qobject")));
        SET_NODE_NAME(m_qobjectServer);

        m_qobjectServer.enableRemoting<EngineSourceAPI>(engine.data());

        engine->setRpm(1234);

        m_qobjectClient.connectToNode(QUrl(QStringLiteral("local:qobject")));
        const QScopedPointer<EngineReplica> engine_r(m_qobjectClient.acquire<EngineReplica>());
        engine_r->waitForSource();

        QCOMPARE(engine_r->rpm(), 1234);
    }

    void apiInProcTest() {
        const QScopedPointer<EngineReplica> engine_r_inProc(m_qobjectServer.acquire<EngineReplica>());
        engine_r_inProc->waitForSource();

        QCOMPARE(engine_r_inProc->rpm(), 1234);
    }

    void clientBeforeServerTest() {

        QRemoteObjectNode d_client;
        d_client.connectToNode(QUrl(QStringLiteral("local:cBST")));
        const QScopedPointer<EngineReplica> engine_d(d_client.acquire<EngineReplica>());

        QRemoteObjectHost d_server(QUrl(QStringLiteral("local:cBST")));
        SET_NODE_NAME(d_server);
        d_server.enableRemoting<EngineSourceAPI>(engine.data());
        QSignalSpy spy(engine_d.data(), SIGNAL(rpmChanged(int)));
        engine->setRpm(50);

        spy.wait();
        QCOMPARE(spy.count(), 1);

        QCOMPARE(engine_d->rpm(), 50);

    }

    void largeDataLocalTest() {
        TestLargeData t;
        m_localCentreServer.enableRemoting(&t, QStringLiteral("large"));

        const QScopedPointer<QRemoteObjectDynamicReplica> rep(m_registryClient.acquire(QStringLiteral("large")));
        rep->waitForSource();
        QVERIFY(rep->isInitialized());
        const QMetaObject *metaObject = rep->metaObject();
        const int sigIndex = metaObject->indexOfSignal("send(QByteArray)");
        QVERIFY(sigIndex != -1);
        const QMetaMethod mm =  metaObject->method(sigIndex);
        QSignalSpy spy(rep.data(), QByteArray(QByteArrayLiteral("2")+mm.methodSignature().constData()));
        const QByteArray data(16384,'y');
        emit t.send(data);
        spy.wait();
        QCOMPARE(spy.count(), 1);
        const QList<QVariant> &arguments = spy.first();
        QVERIFY(arguments.at(0).toByteArray() == data);
        m_localCentreServer.disableRemoting(&t);
    }

    void largeDataTcpTest() {
        TestLargeData t;
        m_basicServer.enableRemoting(&t, QStringLiteral("large"));

        const QScopedPointer<QRemoteObjectDynamicReplica> rep(m_client.acquire(QStringLiteral("large")));
        rep->waitForSource();
        QVERIFY(rep->isInitialized());
        const QMetaObject *metaObject = rep->metaObject();
        const int sigIndex = metaObject->indexOfSignal("send(QByteArray)");
        QVERIFY(sigIndex != -1);
        const QMetaMethod mm =  metaObject->method(sigIndex);
        QSignalSpy spy(rep.data(), QByteArray(QByteArrayLiteral("2")+mm.methodSignature().constData()));
        const QByteArray data(16384,'y');
        emit t.send(data);
        spy.wait();
        QCOMPARE(spy.count(), 1);
        const QList<QVariant> &arguments = spy.first();
        QVERIFY(arguments.at(0).toByteArray() == data);
        m_basicServer.disableRemoting(&t);
    }

    void PODTest()
    {
        MyPOD shouldPass(1, 2.0, QStringLiteral("pass"));
        MyPOD shouldFail(1, 2.0, QStringLiteral("fail"));
        MyClassSimpleSource m;
        m.setMyPOD(shouldPass);
        m_basicServer.enableRemoting(&m);
        const QScopedPointer<MyClassReplica> myclass_r(m_client.acquire<MyClassReplica>());
        myclass_r->waitForSource();

        QVERIFY(myclass_r->myPOD() == m.myPOD());
        QVERIFY(myclass_r->myPOD() != shouldFail);
    }

    void SchemeTest()
    {
        QRemoteObjectHost valid(QUrl(QLatin1String("local:valid")));
        QVERIFY(valid.lastError() == QRemoteObjectNode::NoError);
        QRemoteObjectHost invalid(QUrl(QLatin1String("invalid:invalid")));
        QVERIFY(invalid.lastError() == QRemoteObjectNode::HostUrlInvalid);
        QRemoteObjectNode invalidRegistry(QUrl(QLatin1String("invalid:invalid")));
        QVERIFY(invalidRegistry.lastError() == QRemoteObjectNode::RegistryNotAcquired);
    }

//TODO check Mac support
#ifdef Q_OS_LINUX
    void localServerConnectionTest()
    {
        QProcess testServer;
        const QString progName = QStringLiteral("../localsockettestserver/localsockettestserver");
        //create a fake socket as killing doesn't produce a necessarily unusable socket
        QFile fake(QDir::temp().absoluteFilePath(QStringLiteral("crashMe")));
        fake.remove();
        QVERIFY(fake.open(QFile::Truncate | QFile::WriteOnly));
        QFileInfo info(QDir::temp().absoluteFilePath(QStringLiteral("crashMe")));
        QVERIFY(info.exists());

        QRemoteObjectNode localSocketTestClient;
        const QUrl connection = QUrl(QStringLiteral("local:crashMe"));
        const QString objectname = QStringLiteral("connectme");
        localSocketTestClient.connectToNode(connection);
        QVERIFY(localSocketTestClient.lastError() == QRemoteObjectNode::NoError);
        QScopedPointer<QRemoteObjectDynamicReplica> replica;
        replica.reset(localSocketTestClient.acquire(objectname));

        testServer.start(progName);
        QVERIFY(testServer.waitForStarted());
        QVERIFY(localSocketTestClient.lastError() == QRemoteObjectNode::NoError);
        replica->waitForSource(1000);
        QVERIFY(replica->isInitialized());
        testServer.terminate();
        QVERIFY(testServer.waitForFinished());
    }
    // Tests to take over an existing socket if its still valid
    void localServerConnectionTest2()
    {
        QProcess testServer;
        const QString progName = QStringLiteral("../localsockettestserver/localsockettestserver");

        testServer.start(progName);
        QVERIFY(testServer.waitForStarted());
        QFileInfo info(QDir::temp().absoluteFilePath(QStringLiteral("crashMe")));
        QVERIFY(info.exists());
        testServer.kill();
        testServer.waitForFinished();
        QVERIFY(info.exists());

        QRemoteObjectNode localSocketTestClient;
        const QUrl connection = QUrl(QStringLiteral("local:crashMe"));
        const QString objectname = QStringLiteral("connectme");
        localSocketTestClient.connectToNode(connection);
        QVERIFY(localSocketTestClient.lastError() == QRemoteObjectNode::NoError);
        QScopedPointer<QRemoteObjectDynamicReplica> replica;
        replica.reset(localSocketTestClient.acquire(objectname));

        testServer.start(progName);
        QVERIFY(testServer.waitForStarted());
        QVERIFY(localSocketTestClient.lastError() == QRemoteObjectNode::NoError);
        replica->waitForSource(1000);
        QVERIFY(replica->isInitialized());
        testServer.terminate();
        QVERIFY(testServer.waitForFinished());
    }
#endif

private:
    QScopedPointer<Engine> engine;
    QScopedPointer<Speedometer> speedometer;
    QScopedPointer<TcpDataCenterSimpleSource> dataCenterTcp;
    QScopedPointer<LocalDataCenterSimpleSource> dataCenterLocal;
};

QTEST_MAIN(tst_Integration)

#include "tst_integration.moc"
