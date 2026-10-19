// Copyright (C) 2020 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//#include "rep_pods_replica.h"

#include "simpleobject.h"

#include <QSignalSpy>
#include <QTest>

#include <QByteArray>

class tst_rep_from_header : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testRep_data();
    void testRep();
    void testObjectFromHeader();
};

void tst_rep_from_header::testRep_data()
{
        QTest::addColumn<QString>("actualFile");
        QTest::addColumn<QByteArrayList>("expectedLines");

        QTest::newRow("pods") << QStringLiteral("pods.rep")
                              << QByteArrayList{"POD PodI(int i)",
                                                "POD PodF(float f)",
                                                "POD PodS(QString s)",
                                                "POD PodIFS(int i, float f, QString s)",
                                                ""};
        QTest::newRow("simpleobject") << QStringLiteral("simpleobject.rep")
                                      << QByteArrayList{"class SimpleObject",
                                                        "{",
                                                        "PROP(int value);",
                                                        "}",
                                                        ""};
}

void tst_rep_from_header::testRep()
{
    QFETCH(QString, actualFile);
    QFETCH(QByteArrayList, expectedLines);

    QVERIFY2(QFile::exists(actualFile), qPrintable(actualFile));

    QByteArrayList actualLines;
    {
        QFile f(actualFile);
        QVERIFY(f.open(QIODevice::ReadOnly));
        while (!f.atEnd())
            actualLines.append(f.readLine().trimmed());
    }

    QVERIFY(actualLines == expectedLines);
}

void tst_rep_from_header::testObjectFromHeader()
{
    SimpleObject object;
    QSignalSpy spy(&object, &SimpleObject::valueChanged);
    QVERIFY(object.setProperty("value", 42));
    QCOMPARE(object.value(), 42);
    QCOMPARE(spy.size(), 1);
}

QTEST_APPLESS_MAIN(tst_rep_from_header)

#include "tst_rep_from_header.moc"

