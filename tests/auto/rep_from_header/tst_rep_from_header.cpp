// Copyright (C) 2020 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//#include "rep_pods_replica.h"

#include <QTest>

#include <QByteArray>

class tst_rep_from_header : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testRep_data();
    void testRep();
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

QTEST_APPLESS_MAIN(tst_rep_from_header)

#include "tst_rep_from_header.moc"

