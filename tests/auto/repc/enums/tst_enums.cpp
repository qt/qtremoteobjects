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
        const Qt::DateFormat format1 = Qt::TextDate;
        const Qt::DateFormat format2 = Qt::ISODate;
        const Qt::DateFormat format3 = Qt::SystemLocaleShortDate;
        const Qt::DateFormat format4 = Qt::SystemLocaleLongDate;
        const Qt::DateFormat format5 = Qt::DefaultLocaleShortDate;
        const Qt::DateFormat format6 = Qt::DefaultLocaleLongDate;
        const Qt::DateFormat format7 = Qt::SystemLocaleDate;

        ds << format1 << format2 << format3 << format4 << format5 << format6 << format7;
    }

    ds.device()->seek(0);

    {
        Qt::DateFormat format1, format2, format3, format4, format5, format6, format7;

        ds >> format1 >> format2 >> format3 >> format4 >> format5 >> format6 >> format7;

        QCOMPARE(format1, Qt::TextDate);
        QCOMPARE(format2, Qt::ISODate);
        QCOMPARE(format3, Qt::SystemLocaleShortDate);
        QCOMPARE(format4, Qt::SystemLocaleLongDate);
        QCOMPARE(format5, Qt::DefaultLocaleShortDate);
        QCOMPARE(format6, Qt::DefaultLocaleLongDate);
        QCOMPARE(format7, Qt::SystemLocaleDate);
    }
}

QTEST_APPLESS_MAIN(tst_Enums)

#include "tst_enums.moc"
