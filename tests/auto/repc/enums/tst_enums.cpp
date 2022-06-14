// Copyright (C) 2017-2020 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rep_enums_replica.h"

#include <QTest>

#include <QByteArray>
#include <QDataStream>

class tst_Enums : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testMarshalling();
};

void tst_Enums::testMarshalling()
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::ReadWrite);

    {
        const Qt::DayOfWeek format1 = Qt::Monday;
        const Qt::DayOfWeek format2 = Qt::Tuesday;
        const Qt::DayOfWeek format3 = Qt::Wednesday;
        const Qt::DayOfWeek format4 = Qt::Thursday;
        const Qt::DayOfWeek format5 = Qt::Friday;
        const Qt::DayOfWeek format6 = Qt::Saturday;
        const Qt::DayOfWeek format7 = Qt::Sunday;

        ds << format1 << format2 << format3 << format4 << format5 << format6 << format7;
    }

    ds.device()->seek(0);

    {
        Qt::DayOfWeek format1, format2, format3, format4, format5, format6, format7;

        ds >> format1 >> format2 >> format3 >> format4 >> format5 >> format6 >> format7;

        QCOMPARE(format1, Qt::Monday);
        QCOMPARE(format2, Qt::Tuesday);
        QCOMPARE(format3, Qt::Wednesday);
        QCOMPARE(format4, Qt::Thursday);
        QCOMPARE(format5, Qt::Friday);
        QCOMPARE(format6, Qt::Saturday);
        QCOMPARE(format7, Qt::Sunday);
    }
}

QTEST_APPLESS_MAIN(tst_Enums)

#include "tst_enums.moc"
