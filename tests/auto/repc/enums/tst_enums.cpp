/****************************************************************************
**
** Copyright (C) 2017-2020 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
