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

#include "rep_pods_replica.h"

#include <QTest>

#include <QByteArray>
#include <QDataStream>

class tst_Pods : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testConstructors();
    void testMarshalling();
};


void tst_Pods::testConstructors()
{
    PodI pi1;
    QCOMPARE(pi1.i(), 0);

    PodI pi2(1);
    QCOMPARE(pi2.i(), 1);

    PodI pi3(pi2);
    QCOMPARE(pi3.i(), pi2.i());
}

void tst_Pods::testMarshalling()
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::ReadWrite);

    {
        PodI i1(1), i2(2), i3(3), iDeadBeef(0xdeadbeef);
        ds << i1 << i2 << i3 << iDeadBeef;
    }

    ds.device()->seek(0);

    {
        PodI i1, i2, i3, iDeadBeef;
        ds >> i1 >> i2 >> i3 >> iDeadBeef;

        QCOMPARE(i1.i(), 1);
        QCOMPARE(i2.i(), 2);
        QCOMPARE(i3.i(), 3);
        QCOMPARE(iDeadBeef.i(), int(0xdeadbeef));
    }
}

QTEST_APPLESS_MAIN(tst_Pods)

#include "tst_pods.moc"
