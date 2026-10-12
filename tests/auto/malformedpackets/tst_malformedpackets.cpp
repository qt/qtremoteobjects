// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "rep_mismatch_replica.h"

#include <QtRemoteObjects/QRemoteObjectHost>
#include <QtRemoteObjects/QRemoteObjectNode>
#include <QtTest/QTest>

#include <QtCore/QRegularExpression>

#include <QtNetwork/qtcpserver.h>

#include <memory>

using namespace Qt::StringLiterals;

// The API of this source intentionally does not match the API of the
// replica generated from mismatch.rep.
// The source is registered as a dynamic one, so no signature check is done
// when the replica connects to it.
// The order of the properties, notify signals and slots must match the
// order in mismatch.rep, so that the indices line up.
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

Q_SIGNALS:
    void convertibleValueChanged(const QString &value);
    void nonConvertibleValueChanged(const QString &value);

private:
    QString m_convertibleValue = u"initial"_s;
    QString m_nonConvertibleValue = u"initial"_s;
    QString m_convertibleSlotArgument = u"initial"_s;
    QString m_nonConvertibleSlotArgument = u"initial"_s;
    int m_convertibleSlotCallCount = 0;
    int m_nonConvertibleSlotCallCount = 0;
};

class tst_MalformedPackets : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void slotArgumentTypeMismatch();
    void propertyTypeMismatch();

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

QTEST_MAIN(tst_MalformedPackets)

#include "tst_malformedpackets.moc"
