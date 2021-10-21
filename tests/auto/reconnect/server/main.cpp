/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QCoreApplication>
#include <QtTest/QtTest>
#include <QtRemoteObjects/qremoteobjectnode.h>
#include <QtRemoteObjects/qremoteobjectsource.h>


class SourceObj : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void ping() { ++m_pingCount; };

Q_SIGNALS:
    void pong();

public:
    int m_pingCount = 0;
};

class tst_Server_Process : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testRun()
    {
        const QString url = qEnvironmentVariable("RO_URL");
        QRemoteObjectHost *srcNode = new QRemoteObjectHost(QUrl(url));
        SourceObj so;
        so.setObjectName(QStringLiteral("SourceObj"));
        srcNode->enableRemoting(&so);

        QTRY_VERIFY(so.m_pingCount == 1);
        emit so.pong();
        QTRY_VERIFY(so.m_pingCount == 2);
        delete srcNode;

        srcNode = new QRemoteObjectHost(QUrl(url));
        srcNode->enableRemoting(&so);

        QTRY_VERIFY(so.m_pingCount == 3);
        emit so.pong();
        QTRY_VERIFY(so.m_pingCount == 4);
        delete srcNode;
    }
};

QTEST_MAIN(tst_Server_Process)

#include "main.moc"
