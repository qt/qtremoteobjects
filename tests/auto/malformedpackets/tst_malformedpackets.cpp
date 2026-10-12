// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "rep_mismatch_replica.h"

#include <QtRemoteObjects/QRemoteObjectHost>
#include <QtRemoteObjects/QRemoteObjectNode>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <QtCore/QRegularExpression>

#include <QtNetwork/qtcpserver.h>

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

QTEST_MAIN(tst_MalformedPackets)

#include "tst_malformedpackets.moc"
