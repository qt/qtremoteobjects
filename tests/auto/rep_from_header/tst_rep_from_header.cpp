/****************************************************************************
**
** Copyright (C) 2020 Ford Motor Company
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
    const auto readFile = [&](const QString &fileName) {
        QFile f(fileName);
        f.open(QIODevice::ReadOnly);
        QByteArrayList lines;
        while (!f.atEnd())
            lines.append(f.readLine().trimmed());
        return lines;
    };

    QVERIFY2(QFile::exists(actualFile), qPrintable(actualFile));

    QByteArrayList actualLines = readFile(actualFile);

    QVERIFY(actualLines == expectedLines);
}

QTEST_APPLESS_MAIN(tst_rep_from_header)

#include "tst_rep_from_header.moc"

