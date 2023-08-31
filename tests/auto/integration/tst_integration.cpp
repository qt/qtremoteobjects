// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../../shared/testutils.h"

#include <QtTest/QtTest>
#include <QMetaType>
#if QT_CONFIG(process)
#include <QProcess>
#endif
#include <QFileInfo>
#include <QTcpServer>
#include <QTcpSocket>

#include <QRemoteObjectReplica>
#include <QRemoteObjectNode>
#include <QRemoteObjectSettingsStore>
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

#include "../../shared/testutils.h"

#define SET_NODE_NAME(obj) (obj).setName(QLatin1String(#obj))

//DUMMY impl for variant comparison
bool operator<(const QList<int> &lhs, const QList<int> &rhs)
{
    return lhs.size() < rhs.size();
}

class TestLargeData: public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void send(const QByteArray &data);
};

class TestDynamicBase : public QObject
{
    Q_OBJECT
public:
    TestDynamicBase(QObject *parent=nullptr) : QObject(parent) {}

signals:
    void otherValueChanged();
};


class TestDynamic : public TestDynamicBase
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int otherValue READ otherValue WRITE setOtherValue NOTIFY otherValueChanged)
public:
    TestDynamic(QObject *parent=nullptr) :
        TestDynamicBase(parent),
        m_value(0),
        m_otherValue(0) {}

    int value() const { return m_value; }
    void setValue(int value)
    {
        if (m_value == value)
            return;

        m_value = value;
        emit valueChanged();
    }

    int otherValue() const { return m_otherValue; }
    void setOtherValue(int otherValue)
    {
        if (m_otherValue == otherValue)
            return;

        m_otherValue = otherValue;
        emit otherValueChanged();
    }

signals:
    void valueChanged();

private:
    int m_value;
    int m_otherValue;
};

class TestPersistedStore : public QRemoteObjectAbstractPersistedStore
{
    Q_OBJECT

public:
    TestPersistedStore() : type(EngineReplica::HYBRID) {}
    void saveProperties(const QString &, const QByteArray &, const QVariantList &values) override
    {
        type = values.at(0).value<EngineReplica::EngineType>();
    }
    QVariantList restoreProperties(const QString &, const QByteArray &) override
    {
        return QVariantList() << QVariant::fromValue(type);
    }
private:
    EngineReplica::EngineType type;
};

class tst_Integration: public QObject
{
    Q_OBJECT

    void setupTcp()
    {
        if (!tcpServer) {
            tcpServer = new QTcpServer;
            tcpServer->listen(QHostAddress::Any, 65511);
            socketClient = new QTcpSocket;
            socketClient->connectToHost(QHostAddress::LocalHost, tcpServer->serverPort());
            QVERIFY(socketClient->waitForConnected(5000));

            QVERIFY(tcpServer->waitForNewConnection(5000));
            QVERIFY(tcpServer->hasPendingConnections());
            socketServer = tcpServer->nextPendingConnection();
        }
    }

    void setupHost(bool useRegistry=false)
    {
        QFETCH_GLOBAL(QUrl, hostUrl);
        QFETCH_GLOBAL(QUrl, registryUrl);
        host = new QRemoteObjectHost;
        SET_NODE_NAME(*host);
        if (!hostUrl.isEmpty()) {
            host->setHostUrl(hostUrl);
            if (useRegistry)
                host->setRegistryUrl(registryUrl);
        } else {
            setupTcp();
            host->addHostSideConnection(socketServer);
        }
    }

    void setupClient(bool useRegistry=false)
    {
        QFETCH_GLOBAL(QUrl, hostUrl);
        QFETCH_GLOBAL(QUrl, registryUrl);
        client = new QRemoteObjectNode;
        Q_SET_OBJECT_NAME(*client);
        if (!hostUrl.isEmpty())
        {
            if (useRegistry)
                client->setRegistryUrl(registryUrl);
            else {
                client->connectToNode(hostUrl);
            }
        } else {
            setupTcp();
            client->addClientSideConnection(socketClient);
        }
    }

    void setupRegistry()
    {
        QFETCH_GLOBAL(QUrl, registryUrl);
        registry = new QRemoteObjectRegistryHost(registryUrl);
        SET_NODE_NAME(*registry);
    }

signals:
    void forwardResult(int);

private:
    QRemoteObjectHost *host;
    QRemoteObjectNode *client;
    QRemoteObjectRegistryHost *registry;
    QTcpServer *tcpServer;
    QPointer<QTcpSocket> socketClient, socketServer;

private slots:
    void initTestCase_data()
    {
        QTest::addColumn<QUrl>("hostUrl");
        QTest::addColumn<QUrl>("registryUrl");

        QTest::newRow("tcp") << QUrl(QLatin1String("tcp://127.0.0.1:65511")) << QUrl(QLatin1String("tcp://127.0.0.1:65512"));
#ifdef __QNXNTO__
        QTest::newRow("qnx") << QUrl(QLatin1String("qnx:replica")) << QUrl(QLatin1String("qnx:registry"));
#endif
        QTest::newRow("local") << QUrl(QLatin1String(LOCAL_SOCKET ":replicaLocalIntegration")) << QUrl(QLatin1String(LOCAL_SOCKET ":registryLocalIntegration"));
#ifdef Q_OS_LINUX
        QTest::newRow("localabstract") << QUrl(QLatin1String("localabstract:replicaAbstractIntegration")) << QUrl(QLatin1String("localabstract:registryAbstractIntegration"));
#endif
        QTest::newRow("external") << QUrl() << QUrl();
    }

    void initTestCase()
    {
        QVERIFY(TestUtils::init("integration"));
        QLoggingCategory::setFilterRules("qt.remoteobjects.warning=false");

        // use different paths in QRemoteObjectSettingsStore
        QCoreApplication::setOrganizationName(QLatin1String("QtProject"));
        QStandardPaths::setTestModeEnabled(true);
    }

    void init()
    {
        registry = nullptr;
        host = nullptr;
        client = nullptr;
        tcpServer = nullptr;
        socketClient = nullptr;
        socketServer = nullptr;
    }

    void cleanup()
    {
        delete registry;
        delete host;
        delete client;
        delete tcpServer;
        if (socketClient) {
            socketClient->deleteLater();
        }
        if (socketServer) {
            socketServer->deleteLater();
        }
        // wait for delivery of RemoveObject events to the source
        QTest::qWait(200);
    }

    void basicTest()
    {
        setupHost();
        Engine e;
        e.setRpm(1234);
        host->enableRemoting(&e);

        setupClient();
        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), e.rpm());
    }

    void persistRestoreTest()
    {
        QRemoteObjectNode _client;
        Q_SET_OBJECT_NAME(_client);
        TestPersistedStore store;
        _client.setPersistedStore(&store);

        const QScopedPointer<EngineReplica> engine_r(_client.acquire<EngineReplica>());
        QCOMPARE(engine_r->engineType(), EngineReplica::HYBRID);
    }

    void persistTest()
    {
        QRemoteObjectSettingsStore store;

        setupHost();
        Engine e;
        e.setEngineType(EngineSimpleSource::ELECTRIC);
        host->enableRemoting(&e);

        setupClient();
        client->setPersistedStore(&store);

        QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->engineType(), EngineReplica::ELECTRIC);

        // Delete to persist
        engine_r.reset();
        host->disableRemoting(&e);

        engine_r.reset(client->acquire<EngineReplica>());
        QCOMPARE(engine_r->waitForSource(1000), false);
        QCOMPARE(engine_r->engineType(), EngineReplica::ELECTRIC);
    }

    // ensure we don't crash when ObjectList iterates over in process replicas
    void inProcessObjectList()
    {
        setupRegistry();
        setupHost(true);
        setupClient(true);
        Engine e;
        host->enableRemoting(&e);
        e.setStarted(false);

        const QScopedPointer<EngineReplica> engine_r(host->acquire<EngineReplica>());
        const QScopedPointer<EngineReplica> engine_r2(client->acquire<EngineReplica>());
        engine_r->waitForSource(1000);
        engine_r2->waitForSource(1000);
        QCOMPARE(engine_r->started(), false);
        QCOMPARE(engine_r2->started(), false);
        engine_r->pushStarted(true);

        QTRY_COMPARE(engine_r->started(), true);
        QTRY_COMPARE(engine_r2->started(), true);
    }

    void enumTest()
    {
        setupHost();

        setupClient();

        TestClassSimpleSource tc;
        tc.setTestEnum(TestEnum::FALSE);
        tc.setClassEnum(TestClassSimpleSource::One);
        tc.setClassEnumRW(TestClassSimpleSource::One);
        host->enableRemoting(&tc);
        const QScopedPointer<TestClassReplica> tc_rep(client->acquire<TestClassReplica>());
        tc_rep->waitForSource();
        QCOMPARE(tc.testEnum(), tc_rep->testEnum());
        QCOMPARE(qint32(tc.classEnum()), qint32(TestClassSimpleSource::One));

        // set property on the replica (test property change packet)
        {
            QSignalSpy spy(tc_rep.data(), &TestClassReplica::classEnumChanged);
            QVERIFY(spy.isValid());
            tc_rep->pushClassEnum(TestClassReplica::Two);
            QVERIFY(spy.size() || spy.wait());

            QCOMPARE(qint32(tc.classEnum()), qint32(tc_rep->classEnum()));
        }

        // set property on the source (test property change packet)
        {
            QSignalSpy spy(tc_rep.data(), &TestClassReplica::classEnumChanged);
            tc.setClassEnum(TestClassSimpleSource::One);
            QVERIFY(spy.wait());

            QCOMPARE(qint32(tc.classEnum()), qint32(tc_rep->classEnum()));
        }

        QScopedPointer<QRemoteObjectDynamicReplica> tc_repDynamic(client->acquireDynamic(QStringLiteral("TestClass")));

        tc_repDynamic->waitForSource(1000);
        QVERIFY(tc_repDynamic->isInitialized());

        const QMetaObject *metaObject = tc_repDynamic->metaObject();

        int propertyIndex = metaObject->indexOfProperty("classEnumRW");
        QVERIFY(propertyIndex >= 0);

        QMetaProperty property = metaObject->property(propertyIndex);
        QVERIFY(property.isValid());
        QCOMPARE(property.typeName(), "TestClassReplica::ClassEnum");

        // read enum on the dynamic replica
        {
            QCOMPARE(property.read(tc_repDynamic.data()).value<TestClassReplica::ClassEnum>(), TestClassReplica::One);
        }

        // write enum on the dynamic replica
        {
            QSignalSpy spy(tc_rep.data(), &TestClassReplica::classEnumRWChanged);
            property.write(tc_repDynamic.data(), TestClassReplica::Two);
            QVERIFY(spy.wait());

            QCOMPARE(tc_rep->classEnumRW(), TestClassReplica::Two);
        }

        propertyIndex = metaObject->indexOfProperty("classEnum");
        QVERIFY(propertyIndex >= 0);

        property = metaObject->property(propertyIndex);
        QVERIFY(property.isValid());
        QCOMPARE(property.typeName(), "TestClassReplica::ClassEnum");

        // read enum on the dynamic replica
        {
            QCOMPARE(property.read(tc_repDynamic.data()).value<TestClassReplica::ClassEnum>(), TestClassReplica::One);
        }

        // ensure write enum fails on ReadPush
        {
            QSignalSpy spy(tc_rep.data(), &TestClassReplica::classEnumChanged);
            bool res = property.write(tc_repDynamic.data(), TestClassReplica::Two);
            QVERIFY(!res);
            int methodIndex = metaObject->indexOfMethod("pushClassEnum(TestClassReplica::ClassEnum)");
            QVERIFY(methodIndex >= 0);
            QMetaMethod method = metaObject->method(methodIndex);
            QVERIFY(method.isValid());

            QVERIFY(method.invoke(tc_repDynamic.data(), Q_ARG(TestClassReplica::ClassEnum, TestClassReplica::Two)));
            QVERIFY(spy.wait());

            QCOMPARE(tc_rep->classEnum(), TestClassReplica::Two);
        }
    }

    void namedObjectTest()
    {
        setupHost();

        setupClient();

        Engine e;
        e.setRpm(3333);
        Engine *e2 = new Engine();
        QScopedPointer<Engine> engineSave;
        engineSave.reset(e2);
        e2->setRpm(4444);
        host->enableRemoting(&e);
        host->enableRemoting(e2, QStringLiteral("MyTestEngine"));

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        const QScopedPointer<EngineReplica> namedEngine_r(client->acquire<EngineReplica>(QStringLiteral("MyTestEngine")));
        engine_r->waitForSource();
        QCOMPARE(engine_r->cylinders(), e.cylinders());
        QCOMPARE(engine_r->rpm(), 3333);
        namedEngine_r->waitForSource();
        QCOMPARE(namedEngine_r->cylinders(), e2->cylinders());
        QCOMPARE(namedEngine_r->rpm(), 4444);

        engineSave.reset();
        //Deleting the object before disable remoting will cause disable remoting to
        //return false;
        QVERIFY(!host->disableRemoting(e2));
    }

    void multipleInstancesTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        auto instances = client->instances<EngineReplica>();
        QCOMPARE(instances, QStringList());

        Engine e2;
        host->enableRemoting(&e2, QStringLiteral("Engine2"));

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        const QScopedPointer<EngineReplica> engine2_r(client->acquire<EngineReplica>(QStringLiteral("Engine2")));
        const QScopedPointer<EngineReplica> engine3_r(client->acquire<EngineReplica>(QStringLiteral("Engine_doesnotexist")));
        QVERIFY(engine_r->waitForSource());
        QVERIFY(engine2_r->waitForSource());
        QVERIFY(!engine3_r->waitForSource(500));

        instances = client->instances<EngineReplica>();
        QCOMPARE(instances, QStringList({"Engine", "Engine2"}));

        QSignalSpy spy(engine_r.data(), &QRemoteObjectReplica::stateChanged);
        host->disableRemoting(&e);
        spy.wait();
        QCOMPARE(spy.size(), 1);

        instances = client->instances<EngineReplica>();
        QCOMPARE(instances, QStringList({"Engine2"}));
    }

    void registrySourceLocationBindings()
    {
        QFETCH_GLOBAL(QUrl, registryUrl);
        QFETCH_GLOBAL(QUrl, hostUrl);
        if (registryUrl.isEmpty())
            QSKIP("Skipping registry tests for external QIODevice types.");

        setupRegistry();
        setupHost(true);
        setupClient(true);

        QVERIFY(host->registry()->sourceLocations().empty());
        QVERIFY(client->registry()->sourceLocations().empty());

        QVERIFY(host->registry()->bindableSourceLocations().isReadOnly());
        QVERIFY(client->registry()->bindableSourceLocations().isReadOnly());

        Engine e1;
        const auto engine1 = QStringLiteral("Engine1");
        Engine e2;
        const auto engine2 = QStringLiteral("Engine2");

        QRemoteObjectSourceLocations expectedSourceLocations;
        expectedSourceLocations[engine1] = { QStringLiteral("Engine"), hostUrl };

        int hostSrcLocationsChanged = 0;
        auto hostHandler = host->registry()->bindableSourceLocations().onValueChanged([&] {
            QCOMPARE(host->registry()->sourceLocations(), expectedSourceLocations);
            ++hostSrcLocationsChanged;
        });

        int clientSrcLocationsChanged = 0;
        auto clientHandler = client->registry()->bindableSourceLocations().onValueChanged([&] {
            QCOMPARE(client->registry()->sourceLocations(), expectedSourceLocations);
            ++clientSrcLocationsChanged;
        });

        QProperty<QRemoteObjectSourceLocations> hostObserver;
        hostObserver.setBinding([&] { return host->registry()->sourceLocations(); });

        QProperty<QRemoteObjectSourceLocations> clientObserver;
        clientObserver.setBinding([&] { return client->registry()->sourceLocations(); });

        QSignalSpy hostSpy(host->registry(), &QRemoteObjectRegistry::remoteObjectAdded);
        QSignalSpy clientSpy(client->registry(), &QRemoteObjectRegistry::remoteObjectAdded);

        host->enableRemoting(&e1, engine1);
        QTRY_COMPARE(hostSpy.size(), 1);
        QTRY_COMPARE(clientSpy.size(), 1);
        QCOMPARE(hostObserver.value(), host->registry()->sourceLocations());
        QCOMPARE(clientObserver.value(), client->registry()->sourceLocations());
        QCOMPARE(hostObserver.value(), clientObserver.value());
        QCOMPARE(hostObserver.value(), expectedSourceLocations);
        QCOMPARE(hostSrcLocationsChanged, 1);
        QCOMPARE(clientSrcLocationsChanged, 1);

        expectedSourceLocations[engine2] = { QStringLiteral("Engine"), hostUrl };
        host->enableRemoting(&e2, engine2);
        QTRY_COMPARE(hostSpy.size(), 2);
        QTRY_COMPARE(clientSpy.size(), 2);
        QCOMPARE(hostObserver.value(), host->registry()->sourceLocations());
        QCOMPARE(clientObserver.value(), client->registry()->sourceLocations());
        QCOMPARE(hostObserver.value(), clientObserver.value());
        QCOMPARE(hostObserver.value(), expectedSourceLocations);
        QCOMPARE(hostSrcLocationsChanged, 2);
        QCOMPARE(clientSrcLocationsChanged, 2);

        // Test source removal
        host->disableRemoting(&e1);
        expectedSourceLocations.remove(engine1);
        QSignalSpy srcRemovedHostSpy(host->registry(), &QRemoteObjectRegistry::remoteObjectRemoved);
        QSignalSpy srcRemovedClientSpy(client->registry(),
                                       &QRemoteObjectRegistry::remoteObjectRemoved);

        QTRY_COMPARE(srcRemovedHostSpy.size(), 1);
        QTRY_COMPARE(srcRemovedClientSpy.size(), 1);
        QCOMPARE(hostObserver.value(), host->registry()->sourceLocations());
        QCOMPARE(clientObserver.value(), client->registry()->sourceLocations());
        QCOMPARE(hostObserver.value(), clientObserver.value());
        QCOMPARE(hostObserver.value(), expectedSourceLocations);
        QCOMPARE(hostSrcLocationsChanged, 3);
        QCOMPARE(clientSrcLocationsChanged, 3);
    }

    void registryAddedTest()
    {
        QFETCH_GLOBAL(QUrl, registryUrl);
        if (registryUrl.isEmpty())
            QSKIP("Skipping registry tests for external QIODevice types.");
        setupRegistry();

        setupHost(true);

        setupClient(true);

        QScopedPointer<EngineReplica> regBase, regNamed;
        QScopedPointer<QRemoteObjectDynamicReplica> regDynamic, regDynamicNamed;

        int regAdded = 0;
        connect(client->registry(), &QRemoteObjectRegistry::remoteObjectAdded,
                this, [&](QRemoteObjectSourceLocation entry)
            {
                if (entry.first == QLatin1String("Engine")) {
                    ++regAdded;
                    //Add regular replica first, then dynamic one
                    regBase.reset(client->acquire<EngineReplica>());
                    regDynamic.reset(client->acquireDynamic(QStringLiteral("Engine")));
                }
                if (entry.first == QLatin1String("MyTestEngine")) {
                    regAdded += 2;
                    //Now add dynamic replica first, then regular one
                    regDynamicNamed.reset(client->acquireDynamic(QStringLiteral("MyTestEngine")));
                    regNamed.reset(client->acquire<EngineReplica>(QStringLiteral("MyTestEngine")));
                }
            });

        QSignalSpy addedSpy(client->registry(), &QRemoteObjectRegistry::remoteObjectAdded);

        Engine e;
        e.setRpm(1111);
        host->enableRemoting(&e);
        Engine e2;
        e2.setRpm(2222);
        host->enableRemoting(&e2, QStringLiteral("MyTestEngine"));
        while (regAdded < 3) {
            addedSpy.wait(100);
        }
        regBase->waitForSource(100);
        regNamed->waitForSource(100);
        regDynamic->waitForSource(100);
        regDynamicNamed->waitForSource(100);
        QVERIFY(regBase->isInitialized());
        QCOMPARE(regBase->rpm(),e.rpm());
        QVERIFY(regNamed->isInitialized());
        QCOMPARE(regNamed->rpm(),e2.rpm());

        QVERIFY(regDynamic->isInitialized());
        const QMetaObject *metaObject = regDynamic->metaObject();

        const int propertyIndex = metaObject->indexOfProperty("rpm");
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = metaObject->property(propertyIndex);
        QVERIFY(property.isValid());

        QCOMPARE(property.read(regDynamic.data()).toInt(),e.rpm());

        QVERIFY(regDynamicNamed->isInitialized());
        QCOMPARE(property.read(regDynamicNamed.data()).toInt(),e2.rpm());

        QVERIFY(host->disableRemoting(&e));
        QVERIFY(host->disableRemoting(&e2));
    }

    void registryTest()
    {
        QFETCH_GLOBAL(QUrl, registryUrl);
        if (registryUrl.isEmpty())
            QSKIP("Skipping registry tests for external QIODevice types.");
        setupRegistry();
        TcpDataCenterSimpleSource source1;
        source1.setData1(5);
        source1.setData2(5.0);
        source1.setData3(QStringLiteral("tcp"));
        source1.setData4(QList<int> { 1, 2, 3, 4, 5 });
        registry->enableRemoting(&source1);

        setupHost(true);
        LocalDataCenterSimpleSource source2;
        source2.setData1(5);
        source2.setData2(5.0);
        source2.setData3(QStringLiteral("local"));
        source2.setData4(QList<int> { 1, 2, 3, 4, 5 });
        host->enableRemoting(&source2);
        QVERIFY(host->waitForRegistry(1000));

        setupClient(true);

        const QScopedPointer<TcpDataCenterReplica> tcpCentre(client->acquire<TcpDataCenterReplica>());
        const QScopedPointer<LocalDataCenterReplica> localCentre(client->acquire<LocalDataCenterReplica>());
        QTRY_VERIFY(localCentre->waitForSource(100));
        QTRY_VERIFY(tcpCentre->waitForSource(100));

        QCOMPARE(client->registry()->sourceLocations(), host->registry()->sourceLocations());
        QCOMPARE(client->registry()->sourceLocations(), registry->registry()->sourceLocations());
        QTRY_VERIFY(localCentre->isInitialized());
        QTRY_VERIFY(tcpCentre->isInitialized());

        const QList<int> expected = { 1, 2, 3, 4, 5 };
        QCOMPARE(tcpCentre->data1(), 5 );
        QCOMPARE(tcpCentre->data2(), 5.0);
        QCOMPARE(tcpCentre->data3(), QStringLiteral("tcp"));
        QCOMPARE(tcpCentre->data4(), expected);

        QCOMPARE(localCentre->data1(), 5);
        QCOMPARE(localCentre->data2(), 5.0);
        QCOMPARE(localCentre->data3(), QStringLiteral("local"));
        QCOMPARE(localCentre->data4(), expected);
    }

    void invalidUrlsTest()
    {
        QFETCH_GLOBAL(QUrl, hostUrl);
        QFETCH_GLOBAL(QUrl, registryUrl);
        const QUrl invalidUrl;
        {
            QRemoteObjectHost _host(invalidUrl, registryUrl);
            SET_NODE_NAME(_host);
            const bool res = _host.waitForRegistry(3000);
            QVERIFY(!res);
        }

        {
            QRemoteObjectHost _host(hostUrl, invalidUrl);
            SET_NODE_NAME(_host);
            const bool res = _host.waitForRegistry(3000);
            QVERIFY(!res);
        }

        {
            QRemoteObjectHost _host(invalidUrl, invalidUrl);
            SET_NODE_NAME(_host);
            const bool res = _host.waitForRegistry(3000);
            QVERIFY(!res);
        }
    }

    void noRegistryTest()
    {
        QFETCH_GLOBAL(QUrl, registryUrl);
        if (registryUrl.isEmpty())
            QSKIP("Skipping registry tests for external QIODevice types.");
        setupHost(true);
        const bool res = host->waitForRegistry(3000);
        QVERIFY(!res);
        QCOMPARE(host->registry()->isInitialized(), false);
        const QScopedPointer<Engine> localEngine(new Engine);
        host->enableRemoting(localEngine.data());
        QCOMPARE(host->registry()->sourceLocations().keys().isEmpty(), true);
    }

    void delayedRegistryTest()
    {
        QFETCH_GLOBAL(QUrl, hostUrl);
        QFETCH_GLOBAL(QUrl, registryUrl);
        if (registryUrl.isEmpty())
            QSKIP("Skipping registry tests for external QIODevice types.");
        setupClient(true);

        // create a replica before the registry host started
        // to check whether it gets valid later on
        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        Q_SET_OBJECT_NAME(engine_r.data());
        QTRY_VERIFY(!engine_r->waitForSource(100));

        setupHost(true);
        const bool res = host->waitForRegistry(3000);
        QVERIFY(!res);
        QCOMPARE(host->registry()->isInitialized(), false);

        const QScopedPointer<Engine> localEngine(new Engine);
        host->enableRemoting(localEngine.data());
        QCOMPARE(host->registry()->sourceLocations().keys().isEmpty(), true);

        QSignalSpy spy(host->registry(), &QRemoteObjectRegistry::initialized);
        QSignalSpy addedSpy(host->registry(), &QRemoteObjectRegistry::remoteObjectAdded);
        setupRegistry();
        bool added = addedSpy.wait();
        QVERIFY(spy.size() > 0);
        QCOMPARE(added, true);
        QCOMPARE(host->registry()->sourceLocations().keys().isEmpty(), false);
        QCOMPARE(host->registry()->sourceLocations().keys().at(0), QStringLiteral("Engine"));
        QCOMPARE(host->registry()->sourceLocations().value(QStringLiteral("Engine")).hostUrl, hostUrl);

        // the replicate should be valid now
        QTRY_VERIFY(engine_r->isInitialized());
        QTRY_VERIFY(engine_r->isReplicaValid());

        //This should produce a warning...
        registry->enableRemoting(localEngine.data());
        QVERIFY(host->registry()->sourceLocations().value(QStringLiteral("Engine")).hostUrl != registryUrl);
    }

    void defaultValueTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->cylinders(), 4);
    }

    void notifyTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        QSignalSpy spy(engine_r.data(), &EngineReplica::rpmChanged);
        e.setRpm(2345);

        spy.wait();
        QCOMPARE(spy.size(), 1);
        const QVariantList &arguments = spy.first();
        bool ok;
        int res = arguments.at(0).toInt(&ok);
        QVERIFY(ok);
        QCOMPARE(res, e.rpm());
        QCOMPARE(engine_r->rpm(), e.rpm());
    }

    void dynamicNotifyTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        QSignalSpy spy(this, &tst_Integration::forwardResult);
        QScopedPointer<QRemoteObjectDynamicReplica> engine_dr(client->acquireDynamic(QStringLiteral("Engine")));
        connect(engine_dr.data(), &QRemoteObjectDynamicReplica::initialized, this, [&]()
            {
                const QMetaObject *metaObject = engine_dr->metaObject();
                const int propIndex = metaObject->indexOfProperty("rpm");
                QVERIFY(propIndex >= 0);
                const QMetaProperty mp =  metaObject->property(propIndex);
                QVERIFY(connect(engine_dr.data(), QByteArray(QByteArrayLiteral("2")+mp.notifySignal().methodSignature().constData()), this, SIGNAL(forwardResult(int))));
             });
        e.setRpm(3456);
        spy.wait();
        QCOMPARE(spy.size(), 1);
        const QVariantList &arguments = spy.first();
        bool ok;
        int res = arguments.at(0).toInt(&ok);
        QVERIFY(ok);
        QCOMPARE(res, e.rpm());
    }

    void slotTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();
        e.setStarted(false);

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
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

    void slotTestWithWatcher()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();
        e.setStarted(false);

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        QEventLoop loop;
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        connect(engine_r.data(), &EngineReplica::initialized, &loop, &QEventLoop::quit);
        if (!engine_r->isInitialized())
            loop.exec();
        QCOMPARE(engine_r->started(), false);

        QRemoteObjectPendingReply<bool> reply = engine_r->start();
        QCOMPARE(reply.error(), QRemoteObjectPendingCall::InvalidMessage);

        QRemoteObjectPendingCallWatcher watcher(reply);
        QSignalSpy spy(&watcher, &QRemoteObjectPendingCallWatcher::finished);
        spy.wait();
        QCOMPARE(spy.size(), 1);

        QVERIFY(reply.isFinished());
        QCOMPARE(reply.returnValue(), true);
        QCOMPARE(engine_r->started(), true);
    }

    void slotTestDynamicReplica()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();
        e.setStarted(false);

        const QScopedPointer<QRemoteObjectDynamicReplica> engine_r(client->acquireDynamic(QStringLiteral("Engine")));
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
        QCOMPARE(call.returnValue().metaType(), QMetaType::fromType<bool>());
        QCOMPARE(call.returnValue().toBool(), true);
        started = property.read(engine_r.data()).value<bool>();
        QCOMPARE(started, true);
    }

    void slotTestDynamicReplicaWithArguments()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        const QScopedPointer<QRemoteObjectDynamicReplica> engine_r(client->acquireDynamic(QStringLiteral("Engine")));
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
        QCOMPARE(getCall.returnValue().metaType(), QMetaType::fromType<QString>());
        QCOMPARE(getCall.returnValue().toString(), s);
    }

    void expapiTestDynamicReplica()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        const QScopedPointer<QRemoteObjectDynamicReplica> engine_r(client->acquireDynamic(QStringLiteral("Engine")));
        const QMetaObject *metaObject = engine_r->metaObject();
        const int propIndex = metaObject->indexOfProperty("purchasedPart");
        QVERIFY(propIndex < 0);
        const int methodIndex = metaObject->indexOfMethod("setpurchasedPart(bool)");
        QVERIFY(methodIndex < 0);
    }

    void slotTestInProcess()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);
        e.setStarted(false);

        const QScopedPointer<EngineReplica> engine_r(host->acquire<EngineReplica>());
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
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();

        engine_r->unnormalizedSignature(0, 0);
    }

    void setterTest()
    {
        setupHost();
        Engine e(6);
        QCOMPARE(e.cylinders(), 6);
        host->enableRemoting(&e);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        QCOMPARE(engine_r->cylinders(), 4); // Default value
        engine_r->waitForSource();
        QCOMPARE(engine_r->cylinders(), 6);
        QSignalSpy spy(engine_r.data(), &EngineReplica::rpmChanged);
        engine_r->setRpm(42);
        spy.wait();
        QCOMPARE(spy.size(), 1);
        QCOMPARE(engine_r->rpm(), 42);
    }

    void pushTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->started(), false);
        QSignalSpy spy(engine_r.data(), &EngineReplica::startedChanged);
        engine_r->pushStarted(true);
        spy.wait();
        QCOMPARE(spy.size(), 1);
        QCOMPARE(engine_r->started(), true);
    }

    void dynamicSetterTest()
    {
        setupHost();
        Engine e(6);
        QCOMPARE(e.cylinders(), 6);
        host->enableRemoting(&e);

        setupClient();

        const QScopedPointer<QRemoteObjectDynamicReplica> engine_dr(client->acquireDynamic(QStringLiteral("Engine")));
        engine_dr->waitForSource();
        const QMetaObject *metaObject = engine_dr->metaObject();
        const QMetaProperty const_mp = metaObject->property(metaObject->indexOfProperty("cylinders"));
        QCOMPARE(const_mp.read(engine_dr.data()).toInt(), 6);
        const int propIndex = metaObject->indexOfProperty("rpm");
        const QMetaProperty mp =  metaObject->property(propIndex);
        QSignalSpy spy(engine_dr.data(), QByteArray(QByteArrayLiteral("2")+mp.notifySignal().methodSignature().constData()));
        mp.write(engine_dr.data(), 44);
        spy.wait();
        QCOMPARE(spy.size(), 1);
        QCOMPARE(mp.read(engine_dr.data()).toInt(), 44);
    }

    void slotWithParameterTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);
        e.setRpm(0);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), 0);

        QSignalSpy spy(engine_r.data(), &EngineReplica::rpmChanged);
        engine_r->increaseRpm(1000);
        spy.wait();
        QCOMPARE(spy.size(), 1);
        QCOMPARE(engine_r->rpm(), 1000);
    }

    void slotWithUserReturnTypeTest() {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        e.setTemperature(Temperature(400, QStringLiteral("Kelvin")));

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        QRemoteObjectPendingReply<Temperature> pendingReply = engine_r->temperature();
        pendingReply.waitForFinished();
        Temperature temperature = pendingReply.returnValue();
        QCOMPARE(temperature, Temperature(400, QStringLiteral("Kelvin")));
    }

    void sequentialReplicaTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);

        setupClient();

        e.setRpm(3456);

        QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), e.rpm());

        engine_r.reset(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        QCOMPARE(engine_r->rpm(), e.rpm());
    }

    void doubleReplicaTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);
        e.setRpm(3412);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r1(client->acquire< EngineReplica >());
        const QScopedPointer<EngineReplica> engine_r2(client->acquire< EngineReplica >());

        engine_r1->waitForSource();
        engine_r2->waitForSource();

        QCOMPARE(engine_r1->rpm(), e.rpm());
        QCOMPARE(engine_r2->rpm(), e.rpm());
    }

    // verify that our second replica emits "Changed" signals when initialized
    void doubleReplicaTest2()
    {
        setupHost();
        Engine e;
        host->enableRemoting(&e);
        e.setRpm(3412);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r1(client->acquire< EngineReplica >());
        QSignalSpy spy_r1(engine_r1.data(), &EngineReplica::rpmChanged);
        engine_r1->waitForSource();
        QCOMPARE(engine_r1->rpm(), e.rpm());
        QCOMPARE(spy_r1.size(), 1);

        // NOTE: A second replica will have initialized and notify signals emitted as part of acquire,
        // which leads to different semantics for first and second replicas. Specifically, there is no
        // way to hook in to initialized and the initial notify signals. We should consider changing this.
        const QScopedPointer<EngineReplica> engine_r2(client->acquire< EngineReplica >());
//        QSignalSpy spy_r2(engine_r2.data(), &EngineReplica::rpmChanged);
//        engine_r2->waitForSource();
        QCOMPARE(engine_r2->rpm(), e.rpm());
//        QCOMPARE(spy_r2.count(), 1);
    }

    void twoReplicaTest() {
        setupHost();
        Engine e;
        Speedometer s;
        host->enableRemoting(&e);
        host->enableRemoting(&s);

        setupClient();

        e.setRpm(1234);
        s.setMph(70);

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();
        const QScopedPointer<SpeedometerReplica> speedometer_r(client->acquire<SpeedometerReplica>());
        speedometer_r->waitForSource();

        QCOMPARE(engine_r->rpm(), e.rpm());
        QCOMPARE(speedometer_r->mph(), s.mph());
    }

    void rawDynamicReplicaTest()
    {
        setupHost();
        TestDynamic source;
        host->enableRemoting(&source, "TestDynamic");

        setupClient();

        const QScopedPointer<QRemoteObjectDynamicReplica> replica(client->acquireDynamic(QStringLiteral("TestDynamic")));
        replica->waitForSource();
        QVERIFY(replica->isInitialized());

        QSignalSpy spy(replica.data(), SIGNAL(valueChanged()));

        const QMetaObject *metaObject = replica->metaObject();
        const int propIndex = metaObject->indexOfProperty("value");
        QVERIFY(propIndex != -1);
        const int signalIndex = metaObject->indexOfSignal("valueChanged()");
        QVERIFY(signalIndex != -1);

        // replica gets source change
        source.setValue(1);
        QTRY_COMPARE(spy.size(), 1);
        QCOMPARE(replica->property("value"), QVariant(1));

        // source gets replica change
        replica->setProperty("value", 2);
        QTRY_COMPARE(replica->property("value"), QVariant(2));
        QCOMPARE(source.value(), 2);

        // test parent NOTIFY
        QSignalSpy otherSpy(replica.data(), SIGNAL(otherValueChanged()));

        const int baseSignalIndex = metaObject->indexOfSignal("otherValueChanged()");
        QVERIFY(baseSignalIndex != -1);

        // replica gets source change
        source.setOtherValue(1);
        QTRY_COMPARE(otherSpy.size(), 1);
        QCOMPARE(replica->property("otherValue"), QVariant(1));

        // source gets replica change
        replica->setProperty("otherValue", 2);
        QTRY_COMPARE(replica->property("otherValue"), QVariant(2));
        QCOMPARE(source.otherValue(), 2);
    }

    void dynamicReplicaTest()
    {
        setupHost();
        TcpDataCenterSimpleSource t;
        LocalDataCenterSimpleSource l;
        host->enableRemoting(&t);
        host->enableRemoting(&l);

        setupClient();

        const QScopedPointer<QRemoteObjectDynamicReplica> rep1(client->acquireDynamic(QStringLiteral("TcpDataCenter")));
        const QScopedPointer<QRemoteObjectDynamicReplica> rep2(client->acquireDynamic(QStringLiteral("TcpDataCenter")));
        const QScopedPointer<QRemoteObjectDynamicReplica> rep3(client->acquireDynamic(QStringLiteral("LocalDataCenter")));
        rep1->waitForSource();
        rep2->waitForSource();
        rep3->waitForSource();
        const QMetaObject *metaTcpRep1 = rep1->metaObject();
        const QMetaObject *metaLocalRep1 = rep3->metaObject();
        const QMetaObject *metaTcpSource = t.metaObject();
        const QMetaObject *metaLocalSource = l.metaObject();
        QVERIFY(rep1->isInitialized());
        QVERIFY(rep2->isInitialized());
        QVERIFY(rep3->isInitialized());

        for (int i = 0; i < metaTcpRep1->propertyCount(); ++i)
        {
            const QMetaProperty propLhs =  metaTcpRep1->property(i);
            if (qstrcmp(propLhs.name(), "isReplicaValid") == 0 || qstrcmp(propLhs.name(), "state") == 0 || qstrcmp(propLhs.name(), "node") == 0) //Ignore properties only on the Replica side
                continue;
            const QMetaProperty propRhs =  metaTcpSource->property(metaTcpSource->indexOfProperty(propLhs.name()));
            if (propLhs.notifySignalIndex() == -1)
                QCOMPARE(propRhs.hasNotifySignal(), false);
            else {
                QCOMPARE(propRhs.notifySignalIndex() != -1, true);
                QCOMPARE(metaTcpRep1->method(propLhs.notifySignalIndex()).name(), metaTcpSource->method(propRhs.notifySignalIndex()).name());
            }
            QCOMPARE(propLhs.read(rep1.data()),  propRhs.read(&t));
        }
        for (int i = 0; i < metaLocalRep1->propertyCount(); ++i )
        {
            const QMetaProperty propLhs =  metaLocalRep1->property(i);
            if (qstrcmp(propLhs.name(), "isReplicaValid") == 0 || qstrcmp(propLhs.name(), "state") == 0 || qstrcmp(propLhs.name(), "node") == 0) //Ignore properties only on the Replica side
                continue;
            const QMetaProperty propRhs =  metaLocalSource->property(metaTcpSource->indexOfProperty(propLhs.name()));
            if (propLhs.notifySignalIndex() == -1)
                QCOMPARE(propRhs.hasNotifySignal(), false);
            else {
                QCOMPARE(propRhs.notifySignalIndex() != -1, true);
                QCOMPARE(metaTcpRep1->method(propLhs.notifySignalIndex()).name(), metaTcpSource->method(propRhs.notifySignalIndex()).name());
            }
            QCOMPARE(propLhs.read(rep3.data()),  propRhs.read(&l));
        }

    }

    void apiTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting<EngineSourceAPI>(&e);
        e.setRpm(1234);

        setupClient();

        const QScopedPointer<EngineReplica> engine_r(client->acquire<EngineReplica>());
        engine_r->waitForSource();

        QCOMPARE(engine_r->rpm(), e.rpm());
    }

    void apiInProcTest()
    {
        setupHost();
        Engine e;
        host->enableRemoting<EngineSourceAPI>(&e);
        e.setRpm(1234);

        const QScopedPointer<EngineReplica> engine_r_inProc(host->acquire<EngineReplica>());
        engine_r_inProc->waitForSource();

        QCOMPARE(engine_r_inProc->rpm(), e.rpm());
    }

    void errorSignalTest()
    {
        QRemoteObjectNode _client;
        Q_SET_OBJECT_NAME(_client);
        QSignalSpy errorSpy(&_client, &QRemoteObjectNode::error);
        QVERIFY(!_client.connectToNode(QUrl(QLatin1String("invalid:invalid"))));
        QCOMPARE(errorSpy.size(), 1);
        auto emittedErrorCode = errorSpy.first().at(0).value<QRemoteObjectNode::ErrorCode>();
        QCOMPARE(emittedErrorCode, QRemoteObjectNode::RegistryNotAcquired);
        QCOMPARE(_client.lastError(), QRemoteObjectNode::RegistryNotAcquired);
    }

    void clientBeforeServerTest() {
        setupClient();
        const QScopedPointer<EngineReplica> engine_d(client->acquire<EngineReplica>());

        setupHost();
        Engine e;
        host->enableRemoting<EngineSourceAPI>(&e);
        QSignalSpy spy(engine_d.data(), &EngineReplica::rpmChanged);
        e.setRpm(50);

        spy.wait();
        QCOMPARE(spy.size(), 1);

        QCOMPARE(engine_d->rpm(), e.rpm());
    }

    void largeDataTest()
    {
        TestLargeData t;
        setupHost();
        host->enableRemoting(&t, QStringLiteral("large"));

        setupClient();
        const QScopedPointer<QRemoteObjectDynamicReplica> rep(client->acquireDynamic(QStringLiteral("large")));
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
        QCOMPARE(spy.size(), 1);
        const QVariantList &arguments = spy.first();
        QVERIFY(arguments.at(0).toByteArray() == data);
        QVERIFY(host->disableRemoting(&t));
    }

    void PODTest()
    {
        setupHost();

        setupClient();

        MyPOD shouldPass(1, 2.0, QStringLiteral("pass"));
        MyPOD shouldFail(1, 2.0, QStringLiteral("fail"));
        MyClassSimpleSource m;
        m.setMyPOD(shouldPass);
        host->enableRemoting(&m);
        const QScopedPointer<MyClassReplica> myclass_r(client->acquire<MyClassReplica>());
        myclass_r->waitForSource();

        QVERIFY(myclass_r->myPOD() == m.myPOD());
        QVERIFY(myclass_r->myPOD() != shouldFail);
    }

    void SchemeTest()
    {
        QFETCH_GLOBAL(QUrl, hostUrl);
        QFETCH_GLOBAL(QUrl, registryUrl);
        QRemoteObjectHost valid(hostUrl);
        QVERIFY(valid.lastError() == QRemoteObjectNode::NoError);
        QRemoteObjectHost invalid(QUrl(QLatin1String("invalid:invalid")));
        QVERIFY(invalid.lastError() == QRemoteObjectNode::HostUrlInvalid);
        QRemoteObjectHost validExternal(QUrl(QLatin1String("invalid:invalid")), registryUrl, QRemoteObjectHost::AllowExternalRegistration);
        QVERIFY(validExternal.lastError() == QRemoteObjectNode::NoError);
        QRemoteObjectNode invalidRegistry(QUrl(QLatin1String("invalid:invalid")));
        QVERIFY(invalidRegistry.lastError() == QRemoteObjectNode::RegistryNotAcquired);
    }

#if QT_CONFIG(process) && (defined(Q_OS_LINUX) || defined(Q_OS_DARWIN))
    void localServerConnectionTest()
    {
        QFETCH_GLOBAL(QUrl, hostUrl);
        if (hostUrl.scheme() != QRemoteObjectStringLiterals::local())
            QSKIP("Skipping 'local' specific backend for non-local test.");
        const auto progName = TestUtils::findExecutable("localsockettestserver", "/localsockettestserver");

        //create a fake socket as killing doesn't produce a necessarily unusable socket
        QFile fake(QDir::temp().absoluteFilePath(QStringLiteral("crashMe")));
        fake.remove();
        QVERIFY(fake.open(QFile::Truncate | QFile::WriteOnly));
        QFileInfo info(QDir::temp().absoluteFilePath(QStringLiteral("crashMe")));
        QVERIFY(info.exists());

        QRemoteObjectNode localSocketTestClient;
        const QUrl connection = QUrl(QStringLiteral(LOCAL_SOCKET ":crashMe"));
        const QString objectname = QStringLiteral("connectme");
        localSocketTestClient.connectToNode(connection);
        QVERIFY(localSocketTestClient.lastError() == QRemoteObjectNode::NoError);
        QScopedPointer<QRemoteObjectDynamicReplica> replica;
        replica.reset(localSocketTestClient.acquireDynamic(objectname));

        QProcess testServer;
        testServer.start(progName, QStringList());
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
        QFETCH_GLOBAL(QUrl, hostUrl);
        if (hostUrl.scheme() != QRemoteObjectStringLiterals::local())
            QSKIP("Skipping 'local' specific backend for non-local test.");
        const auto progName = TestUtils::findExecutable("localsockettestserver", "/localsockettestserver");

        QProcess testServer;
        testServer.start(progName, QStringList());
        QVERIFY(testServer.waitForStarted());
        QFileInfo info(QDir::temp().absoluteFilePath(QStringLiteral("crashMe")));
        QVERIFY(info.exists());
        testServer.kill();
        testServer.waitForFinished();
        QVERIFY(info.exists());

        QRemoteObjectNode localSocketTestClient;
        const QUrl connection = QUrl(QStringLiteral(LOCAL_SOCKET ":crashMe"));
        const QString objectname = QStringLiteral("connectme");
        localSocketTestClient.connectToNode(connection);
        QVERIFY(localSocketTestClient.lastError() == QRemoteObjectNode::NoError);
        QScopedPointer<QRemoteObjectDynamicReplica> replica;
        replica.reset(localSocketTestClient.acquireDynamic(objectname));

        testServer.start(progName, QStringList());
        QVERIFY(testServer.waitForStarted());
        QVERIFY(localSocketTestClient.lastError() == QRemoteObjectNode::NoError);
        replica->waitForSource(1000);
        QVERIFY(replica->isInitialized());
        testServer.terminate();
        QVERIFY(testServer.waitForFinished());
    }
#endif

    void tcpListenFailedTest()
    {
        QFETCH_GLOBAL(QUrl, registryUrl);

        if (registryUrl.scheme() != QRemoteObjectStringLiterals::tcp())
            QSKIP("Skipping test for local and external backends.");

        // Need the Host or Registry running so that the port is in use.
        setupRegistry();
        QRemoteObjectHost badHost;
        badHost.setHostUrl(registryUrl);
        QCOMPARE(badHost.lastError(), QRemoteObjectNode::ListenFailed);

    }

    void invalidExternalTest()
    {
        QFETCH_GLOBAL(QUrl, hostUrl);
        if (hostUrl.scheme() != QRemoteObjectStringLiterals::tcp())
            QSKIP("Skipping test for tcp and external backends.");
        QRemoteObjectHost srcNode;
        QTest::ignoreMessage(QtWarningMsg, " Overriding a valid QtRO url ( QUrl(\"tcp://127.0.0.1:65511\") ) with AllowExternalRegistration is not allowed.");
        srcNode.setHostUrl(hostUrl, QRemoteObjectHost::AllowExternalRegistration);
        QCOMPARE(srcNode.lastError(), QRemoteObjectNode::HostUrlInvalid);
        Engine e;
        bool res = srcNode.enableRemoting(&e);
        QVERIFY(res == false);
    }

    void startClientWithoutHost()
    {
        setupClient();
        QScopedPointer<EngineReplica> replica(client->acquire<EngineReplica>());
        client->setHeartbeatInterval(10);
        // Wait, to make sure there's no crash (QTBUG-94513)
        QTest::qWait(200);

        // Make sure creating the host afterwards works
        setupHost();
        Engine e;
        e.setRpm(42);
        host->enableRemoting(&e);

        QVERIFY(replica->waitForSource());
        QCOMPARE(replica->rpm(), e.rpm());
    }
};

QTEST_MAIN(tst_Integration)

#include "tst_integration.moc"
