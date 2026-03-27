// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QString>
#include <QtTest>
#include "rep_stringliteral_merged.h"

class StringLiteralTest : public QObject
{
    Q_OBJECT

public:
    StringLiteralTest() = default;

private Q_SLOTS:
    void basicFunctions();
    void basicFunctions_data();
};

void StringLiteralTest::basicFunctions_data()
{
    QTest::addColumn<bool>("templated");
    QTest::newRow("non-templated enableRemoting") << false;
    QTest::newRow("templated enableRemoting") << true;
}

void StringLiteralTest::basicFunctions()
{
    QFETCH(bool, templated);

    QRemoteObjectRegistryHost host(QUrl("tcp://localhost:5550"));
    StringLiteralSimpleSource source;
    if (templated)
        host.enableRemoting<StringLiteralSourceAPI>(&source);
    else
        host.enableRemoting(&source);

    QRemoteObjectNode client(QUrl("tcp://localhost:5550"));
    const QScopedPointer<StringLiteralReplica> replica(client.acquire<StringLiteralReplica>());

    QVERIFY(replica->waitForSource(300));
    QCOMPARE(replica->name(), u"Default"_s);

    QSignalSpy nameChangeSpy(replica.get(), &StringLiteralReplica::nameChanged);
    source.setName(u"Changed"_s);
    QVERIFY(nameChangeSpy.wait());
    QCOMPARE(replica->name(), u"Changed"_s);
}

QTEST_MAIN(StringLiteralTest)

#include "tst_stringliteraltest.moc"
