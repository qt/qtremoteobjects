// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "rep_mismatch_replica.h"

#include <QtRemoteObjects/QRemoteObjectHost>
#include <QtRemoteObjects/QRemoteObjectNode>
#include <QtRemoteObjects/private/qconnectionfactories_p.h>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <QtCore/QDataStream>
#include <QtCore/QRegularExpression>

#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>

#include <memory>

using namespace Qt::StringLiterals;

// The API of this source intentionally does not match the API of the
// replica generated from mismatch.rep.
// The source is registered as a dynamic one, so no signature check is done
// when the replica connects to it.
// The order of the properties, signals and slots must match the order in
// mismatch.rep, so that the indices line up. The notify signals must come
// first, followed by the other signals in the order of SIGNAL()s.
class MismatchedSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString convertibleValue READ convertibleValue WRITE setConvertibleValue
               NOTIFY convertibleValueChanged)
    Q_PROPERTY(QString nonConvertibleValue READ nonConvertibleValue WRITE setNonConvertibleValue
               NOTIFY nonConvertibleValueChanged)

public:
    using QObject::QObject;

    QString convertibleValue() const { return m_convertibleValue; }
    void setConvertibleValue(const QString &value)
    {
        if (m_convertibleValue == value)
            return;
        m_convertibleValue = value;
        emit convertibleValueChanged(m_convertibleValue);
    }

    QString nonConvertibleValue() const { return m_nonConvertibleValue; }
    void setNonConvertibleValue(const QString &value)
    {
        if (m_nonConvertibleValue == value)
            return;
        m_nonConvertibleValue = value;
        emit nonConvertibleValueChanged(m_nonConvertibleValue);
    }

    int convertibleSlotCallCount() const { return m_convertibleSlotCallCount; }
    QString convertibleSlotArgument() const { return m_convertibleSlotArgument; }

    int nonConvertibleSlotCallCount() const { return m_nonConvertibleSlotCallCount; }
    QString nonConvertibleSlotArgument() const { return m_nonConvertibleSlotArgument; }

    int tooManyArgsSlotCallCount() const { return m_tooManyArgsSlotCallCount; }
    int tooFewArgsSlotCallCount() const { return m_tooFewArgsSlotCallCount; }

public Q_SLOTS:
    void convertibleSlot(const QString &arg)
    {
        ++m_convertibleSlotCallCount;
        m_convertibleSlotArgument = arg;
    }

    void nonConvertibleSlot(const QString &arg)
    {
        ++m_nonConvertibleSlotCallCount;
        m_nonConvertibleSlotArgument = arg;
    }

    // The replica sends two arguments
    void tooManyArgsSlot(int) { ++m_tooManyArgsSlotCallCount; }
    // The replica sends one argument
    void tooFewArgsSlot(int, int) { ++m_tooFewArgsSlotCallCount; }

    void noArgSlot() {}

Q_SIGNALS:
    void convertibleValueChanged(const QString &value);
    void nonConvertibleValueChanged(const QString &value);

    // The replica signal has one argument
    void tooManyArgsSignal(int a, int b);
    // The replica signal has two arguments
    void tooFewArgsSignal(int a);
    // The replica signal has one argument
    void noArgsSignal();

private:
    QString m_convertibleValue = u"initial"_s;
    QString m_nonConvertibleValue = u"initial"_s;
    QString m_convertibleSlotArgument = u"initial"_s;
    QString m_nonConvertibleSlotArgument = u"initial"_s;
    int m_convertibleSlotCallCount = 0;
    int m_nonConvertibleSlotCallCount = 0;
    int m_tooManyArgsSlotCallCount = 0;
    int m_tooFewArgsSlotCallCount = 0;
};

class tst_MalformedPackets : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void slotArgumentTypeMismatch();
    void propertyTypeMismatch();
    void slotArgumentCountMismatch();
    void signalArgumentCountMismatch();
    void registrySlotWithNoArgs();
    void writePropertyWithNoArgs();
    void propertyChangeWithInvalidIndex_data();
    void propertyChangeWithInvalidIndex();

    void propertyCountMismatch_data();
    void propertyCountMismatch();
    void childPropertyCountMismatch_data();
    void childPropertyCountMismatch();
private:

    std::unique_ptr<MismatchedSource> m_source;
    std::unique_ptr<QRemoteObjectHost> m_host;
    std::unique_ptr<QRemoteObjectNode> m_client;
    std::unique_ptr<MismatchedObjectReplica> m_replica;
};

static quint16 findPort()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0))
        return 0;
    quint16 port = probe.serverPort();
    probe.close();
    return port;
}

void tst_MalformedPackets::init()
{
    qint16 port = findPort();
    QVERIFY(port != 0);

    const QUrl url(u"tcp://127.0.0.1:%1"_s.arg(port));

    m_source = std::make_unique<MismatchedSource>();
    m_source->setObjectName(u"MismatchedObject"_s);

    m_host = std::make_unique<QRemoteObjectHost>(url);
    QVERIFY(m_host->enableRemoting(m_source.get()));

    m_client = std::make_unique<QRemoteObjectNode>();
    QVERIFY(m_client->connectToNode(url));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"provided type QString .* QRectF"_s));
    m_replica.reset(m_client->acquire<MismatchedObjectReplica>());
    QVERIFY(m_replica);
    QVERIFY(m_replica->waitForSource());
}

void tst_MalformedPackets::cleanup()
{
    m_replica.reset();
    m_client.reset();
    m_host.reset();
    m_source.reset();
}

void tst_MalformedPackets::slotArgumentTypeMismatch()
{
    // The source expects a QString, but receives an int, which is converted
    {
        m_replica->convertibleSlot(42);

        QTRY_COMPARE(m_source->convertibleSlotCallCount(), 1);
        QCOMPARE(m_source->convertibleSlotArgument(), u"42"_s);
    }
    // The source expects a QString, but receives a QRectF, which cannot be
    // converted, so the slot is invoked with a default-constructed QString
    {
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression(u"provided type QRectF .* QString"_s));
        m_replica->nonConvertibleSlot(QRectF(1, 2, 3, 4));

        QTRY_COMPARE(m_source->nonConvertibleSlotCallCount(), 1);
        QCOMPARE(m_source->nonConvertibleSlotArgument(), QString());
    }
}

void tst_MalformedPackets::propertyTypeMismatch()
{
    // The source property is a QString, but the replica sends an int,
    // which is converted
    {
        // Also make sure that the signal from the source to replica is
        // handled properly
        int value = 99; // some non-default value
        connect(m_replica.get(), &MismatchedObjectReplica::convertibleValueChanged,
                this, [&value](int newVal) { value = newVal; });

        m_replica->setConvertibleValue(42);

        QTRY_COMPARE(m_source->convertibleValue(), u"42"_s);
        QTRY_COMPARE(m_replica->convertibleValue(), 42);
        QTRY_COMPARE(value, 42);
    }
    // The source property is a QString, but the replica sends a QRectF,
    // which cannot be converted, so the property is reset to a
    // default-constructed QString
    {
        // Also make sure that the signal from the source to replica is
        // handled properly
        QRectF value = QRectF(5, 5, 5, 5); // some non-default value
        connect(m_replica.get(), &MismatchedObjectReplica::nonConvertibleValueChanged,
                this, [&value](QRectF newVal) { value = newVal; });

        // Ignore QRectF -> QString on the source when writing the property
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression(u"provided type QRectF .* QString"_s));
        // Ignore QString -> QRectF on the replica when handling PropertyChangePacket
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression(u"provided type QString .* QRectF"_s));
        // Ignore QString -> QRectF on the replica when handling InvokePacket
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression(u"provided type QString .* QRectF"_s));

        m_replica->setNonConvertibleValue(QRectF(1, 2, 3, 4));

        QTRY_COMPARE(m_source->nonConvertibleValue(), QString());
        QTRY_COMPARE(m_replica->nonConvertibleValue(), QRectF());
        QTRY_COMPARE(value, QRectF());
    }
}

void tst_MalformedPackets::slotArgumentCountMismatch()
{
    // The packets are handled in order, so we call a "normal" convertibleSlot()
    // to make sure that the previous call is handled.

    // The replica sends more arguments than the source slot takes
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Argument count mismatch"_s));
        m_replica->tooManyArgsSlot(1, 2);
        m_replica->convertibleSlot(1);

        QTRY_COMPARE(m_source->convertibleSlotCallCount(), 1);
        QCOMPARE(m_source->tooManyArgsSlotCallCount(), 0);
    }
    // The replica sends fewer arguments than the source slot takes
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Argument count mismatch"_s));
        m_replica->tooFewArgsSlot(1);
        m_replica->convertibleSlot(2);

        QTRY_COMPARE(m_source->convertibleSlotCallCount(), 2);
        QCOMPARE(m_source->tooFewArgsSlotCallCount(), 0);
    }
}

void tst_MalformedPackets::signalArgumentCountMismatch()
{
    // The packets are handled in order, so we update a property to make sure
    // that the malformed signal has been handled.

    // The source emits more arguments than the replica signal takes.
    // This does not raise a warning and actually emits the signal on the
    // replica side for backwards compatibility, because there was a separate
    // bug on the Source side.
    {
        QSignalSpy spy(m_replica.get(), &MismatchedObjectReplica::tooManyArgsSignal);
        emit m_source->tooManyArgsSignal(1, 2);
        m_source->setConvertibleValue(u"1"_s);

        QTRY_COMPARE(m_replica->convertibleValue(), 1);
        QCOMPARE(spy.size(), 1);
    }
    // The source emits fewer arguments than the replica signal takes
    {
        QSignalSpy spy(m_replica.get(), &MismatchedObjectReplica::tooFewArgsSignal);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Argument count mismatch"_s));
        emit m_source->tooFewArgsSignal(1);
        m_source->setConvertibleValue(u"2"_s);

        QTRY_COMPARE(m_replica->convertibleValue(), 2);
        QCOMPARE(spy.size(), 0);
    }
    // Same as above, but the source signal has no arguments at all. This is
    // handled by a separate code path on the replica side.
    {
        QSignalSpy spy(m_replica.get(), &MismatchedObjectReplica::noArgsSignal);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Argument count mismatch"_s));
        emit m_source->noArgsSignal();
        m_source->setConvertibleValue(u"3"_s);

        QTRY_COMPARE(m_replica->convertibleValue(), 3);
        QCOMPARE(spy.size(), 0);
    }
}

void tst_MalformedPackets::registrySlotWithNoArgs()
{
    MismatchedSource sourceAsRegistry;
    sourceAsRegistry.setObjectName("Registry");
    m_host->enableRemoting(&sourceAsRegistry);

    // "Registry" is a reserved name, but the module still allows registering
    // custom objects under this name.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"provided type QString .* QRectF"_s));
    std::unique_ptr<MismatchedObjectReplica> registryReplica(
            m_client->acquire<MismatchedObjectReplica>("Registry"));
    QVERIFY(registryReplica);
    QVERIFY(registryReplica->waitForSource());

    // The remote objects Registry always expects a slot to have at least one
    // argument - the QRemoteObjectSourceLocation instance.
    // Calling a slot with no arguments should not crash!
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Unexpected packet format"_s));
    registryReplica->noArgSlot();

    // Call a "normal" slot of another replica (that is registered under a
    // "normal" name) to make sure that the first call is actually processed.
    m_replica->convertibleSlot(1);
    QTRY_COMPARE(m_source->convertibleSlotCallCount(), 1);
}

void tst_MalformedPackets::writePropertyWithNoArgs()
{
    // A hand-crafted property change packet with no arguments.
    // The format matches QDataStreamCodec::serializeInvokePacket().
    QByteArray packet;
    {
        QDataStream ds(&packet, QIODevice::WriteOnly);
        ds.setVersion(QtRemoteObjects::dataStreamVersion);
        ds.setByteOrder(QDataStream::LittleEndian);

        const QMetaObject &mo = MismatchedSource::staticMetaObject;
        const int propertyIndex = mo.indexOfProperty("convertibleValue") - mo.propertyOffset();

        ds << quint32(0); // packet size, filled in below
        ds << quint16(QtRemoteObjects::InvokePacket);
        ds << m_source->objectName();
        ds << int(QMetaObject::WriteProperty);
        ds << propertyIndex;
        ds << quint32(0); // argument count
        ds << int(-1); // serialId
        ds << int(-1); // propertyIndex
        QCOMPARE(ds.status(), QDataStream::Ok);

        ds.device()->seek(0);
        ds << quint32(packet.size() - sizeof(quint32));
    }

    const QUrl url = m_host->hostUrl();

    QTcpSocket socket;
    socket.connectToHost(url.host(), url.port());
    QVERIFY(socket.waitForConnected());

    // Create a new client using the custom connection
    QRemoteObjectNode client;
    client.addClientSideConnection(&socket);

    // Acquire a replica
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"provided type QString .* QRectF"_s));
    std::unique_ptr<MismatchedObjectReplica> replica(client.acquire<MismatchedObjectReplica>());
    QVERIFY(replica);
    QVERIFY(replica->waitForSource());

    // Now send a custom malformed packet - the host should not crash!
    // It should simply discard the command.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Invalid argument count"_s));
    QCOMPARE(socket.write(packet), packet.size());
    QVERIFY(socket.waitForBytesWritten());

    // After that update the property in a normal way to verify that the
    // previous command was processed.
    replica->setConvertibleValue(42);
    QTRY_COMPARE(m_source->convertibleValue(), u"42"_s);
}

void tst_MalformedPackets::propertyChangeWithInvalidIndex_data()
{
    QTest::addColumn<int>("propertyIndex");

    const QMetaObject &mo = MismatchedObjectReplica::staticMetaObject;
    const int propertyCount = mo.propertyCount() - mo.propertyOffset();

    QTest::newRow("equal_to_count") << propertyCount;
    QTest::newRow("large") << propertyCount + 1000;
    QTest::newRow("negative") << -1;
}

void tst_MalformedPackets::propertyChangeWithInvalidIndex()
{
    QFETCH(const int, propertyIndex);

    // A hand-crafted property change packet with an out-of-range index.
    // The format matches QDataStreamCodec::serializePropertyChangePacket().
    QByteArray packet;
    {
        QDataStream ds(&packet, QIODevice::WriteOnly);
        ds.setVersion(QtRemoteObjects::dataStreamVersion);
        ds.setByteOrder(QDataStream::LittleEndian);

        ds << quint32(0); // packet size, filled in below
        ds << quint16(QtRemoteObjects::PropertyChangePacket);
        ds << m_source->objectName();
        ds << propertyIndex;
        ds << QVariant(42);
        QCOMPARE(ds.status(), QDataStream::Ok);

        ds.device()->seek(0);
        ds << quint32(packet.size() - sizeof(quint32));
    }

    // Create the connection manually, so that we can write to the host side
    // of it, and the replica sees the data as if it was sent by the source.
    quint16 port = findPort();
    QVERIFY(port);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, port));

    QTcpSocket clientSocket;
    clientSocket.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(server.waitForNewConnection(5000));
    QTcpSocket *hostSocket = server.nextPendingConnection();
    QVERIFY(hostSocket);

    m_host->addHostSideConnection(hostSocket);

    QRemoteObjectNode client;
    client.addClientSideConnection(&clientSocket);

    // Acquire a replica. The source and the replica have the same number of
    // properties, so the initialization succeeds.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"provided type QString .* QRectF"_s));
    std::unique_ptr<MismatchedObjectReplica> replica(client.acquire<MismatchedObjectReplica>());
    QVERIFY(replica);
    QVERIFY(replica->waitForSource());

    // Now send a custom malformed packet from the host side - the replica
    // should not crash! It should simply discard the packet.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Invalid property index"_s));
    QCOMPARE(hostSocket->write(packet), packet.size());
    QVERIFY(hostSocket->waitForBytesWritten());

    // After that update the property on the source in a normal way. The
    // packets are handled in order, so this verifies that the previous packet
    // was processed.
    m_source->setConvertibleValue(u"1"_s);
    QTRY_COMPARE(replica->convertibleValue(), 1);
}

// Source objects to match the TwoPropertyObject from the replica. One of them
// has less properties, the other one - more.
class OnePropertySource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ prop1 WRITE setProp1 NOTIFY prop1Changed)

public:
    using QObject::QObject;

    int prop1() const { return m_prop1; }
    void setProp1(int val)
    {
        if (m_prop1 == val)
            return;
        m_prop1 = val;
        Q_EMIT prop1Changed(m_prop1);
    }

signals:
    void prop1Changed(int);

private:
    int m_prop1 = 0;
};

class ThreePropertiesSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ prop1 WRITE setProp1 NOTIFY prop1Changed)
    Q_PROPERTY(int prop2 READ prop2 WRITE setProp2 NOTIFY prop2Changed)
    Q_PROPERTY(int prop3 READ prop3 WRITE setProp3 NOTIFY prop3Changed)

public:
    using QObject::QObject;

    int prop1() const { return m_prop1; }
    void setProp1(int val)
    {
        if (m_prop1 == val)
            return;
        m_prop1 = val;
        Q_EMIT prop1Changed(m_prop1);
    }

    int prop2() const { return m_prop2; }
    void setProp2(int val)
    {
        if (m_prop2 == val)
            return;
        m_prop2 = val;
        Q_EMIT prop2Changed(m_prop2);
    }

    int prop3() const { return m_prop3; }
    void setProp3(int val)
    {
        if (m_prop3 == val)
            return;
        m_prop3 = val;
        Q_EMIT prop3Changed(m_prop3);
    }

signals:
    void prop1Changed(int);
    void prop2Changed(int);
    void prop3Changed(int);

private:
    int m_prop1 = 0;
    int m_prop2 = 0;
    int m_prop3 = 0;
};

enum class MismatchType
{
    ReplicaHasLessProperties,
    ReplicaHasMoreProperties,
};

void tst_MalformedPackets::propertyCountMismatch_data()
{
    QTest::addColumn<MismatchType>("type");

    QTest::newRow("replica_has_less_properties") << MismatchType::ReplicaHasLessProperties;
    QTest::newRow("replica_has_more_properties") << MismatchType::ReplicaHasMoreProperties;
}

void tst_MalformedPackets::propertyCountMismatch()
{
    QFETCH(const MismatchType, type);

    // Create both sources
    OnePropertySource src1;
    ThreePropertiesSource src3;
    // But enable remoting for only one of them
    switch (type) {
    case MismatchType::ReplicaHasLessProperties:
        src1.setObjectName("PropertyMismatch");
        m_host->enableRemoting(&src1);
        break;
    case MismatchType::ReplicaHasMoreProperties:
        src3.setObjectName("PropertyMismatch");
        m_host->enableRemoting(&src3);
        break;
    }

    // Acquire a replica that has a different number of properties.
    // This should not crash!
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Property count mismatch"_s));
    std::unique_ptr<TwoPropertyObjectReplica> replica(
            m_client->acquire<TwoPropertyObjectReplica>("PropertyMismatch"));
    QVERIFY(replica);
    QTRY_COMPARE(replica->state(), QRemoteObjectReplica::SignatureMismatch);
}

// The source child objects for the ParentWithChild replica. The type name is
// set explicitly, so that it matches the TwoPropertyObject class from
// mismatch.rep. The replica side needs it to find the metaobject for the new
// child replica when the child pointer changes on the source side.
class OnePropertyChildSource : public QObject
{
    Q_OBJECT
    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, "TwoPropertyObject")
    Q_PROPERTY(int prop1 READ prop1 CONSTANT)

public:
    using QObject::QObject;

    int prop1() const { return 1; }
};

class ThreePropertiesChildSource : public QObject
{
    Q_OBJECT
    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, "TwoPropertyObject")
    Q_PROPERTY(int prop1 READ prop1 CONSTANT)
    Q_PROPERTY(int prop2 READ prop2 CONSTANT)
    Q_PROPERTY(int prop3 READ prop3 CONSTANT)

public:
    using QObject::QObject;

    int prop1() const { return 1; }
    int prop2() const { return 2; }
    int prop3() const { return 3; }
};

// The objectName must match the class name in mismatch.rep, because the
// generated replica uses it to build the name of the child replica.
class ParentWithChildSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value NOTIFY valueChanged)
    Q_PROPERTY(QObject *child READ child NOTIFY childChanged)

public:
    explicit ParentWithChildSource(QObject *child) : m_child(child)
    {
        setObjectName(u"ParentWithChild"_s);
    }

    int value() const { return 42; }

    QObject *child() const { return m_child; }
    void setChild(QObject *child)
    {
        if (m_child == child)
            return;
        m_child = child;
        Q_EMIT childChanged();
    }

Q_SIGNALS:
    void valueChanged();
    void childChanged();

private:
    QObject *m_child = nullptr;
};

static std::unique_ptr<QObject> createChildSource(MismatchType type)
{
    switch (type) {
    case MismatchType::ReplicaHasLessProperties:
        return std::make_unique<ThreePropertiesChildSource>();
    case MismatchType::ReplicaHasMoreProperties:
        return std::make_unique<OnePropertyChildSource>();
    }
    return nullptr;
}

void tst_MalformedPackets::childPropertyCountMismatch_data()
{
    propertyCountMismatch_data();
}

void tst_MalformedPackets::childPropertyCountMismatch()
{
    QFETCH(const MismatchType, type);

    const auto child = createChildSource(type);
    ParentWithChildSource parent(child.get());
    QVERIFY(m_host->enableRemoting(&parent));

    // The child replica has a different number of properties than the source
    // child. This should not crash! The child replica is not initialized,
    // but the parent replica still is.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Property count mismatch for .*"
                                                           "Class::ParentWithChild::child"_s));
    std::unique_ptr<ParentWithChildReplica> replica(
            m_client->acquire<ParentWithChildReplica>());
    QVERIFY(replica);
    QTRY_COMPARE(replica->state(), QRemoteObjectReplica::Valid);
    QCOMPARE(replica->value(), 42);

    TwoPropertyObjectReplica *childReplica = replica->child();
    QVERIFY(childReplica);
    QCOMPARE(childReplica->state(), QRemoteObjectReplica::SignatureMismatch);

    // Changing the child pointer after the parent is initialized makes the
    // replica side acquire a new dynamic replica for the child. Its property
    // count does not match either. This should not crash either!
    const auto newChild = createChildSource(type);
    QSignalSpy childChangedSpy(replica.get(), &ParentWithChildReplica::childChanged);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Property count mismatch for .*"
                                                           "Class::ParentWithChild::child"_s));
    parent.setChild(newChild.get());
    QTRY_COMPARE(childChangedSpy.size(), 1);

    TwoPropertyObjectReplica *newChildReplica = replica->child();
    QVERIFY(newChildReplica);
    QVERIFY(childReplica != newChildReplica);
    QCOMPARE(newChildReplica->state(), QRemoteObjectReplica::SignatureMismatch);
}

QTEST_MAIN(tst_MalformedPackets)

#include "tst_malformedpackets.moc"
