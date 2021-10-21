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

#include <QtTest/QtTest>
#include <QStandardPaths>
#include <QProcess>


static QString findExecutable(const QString &executableName, const QString &searchPath)
{
    const QString path = QStandardPaths::findExecutable(executableName, { searchPath });
    if (!path.isEmpty())
        return path;

    qWarning() << "Could not find executable:" << executableName << "in" << searchPath;
    return QString();
}


class tst_Reconnect: public QObject
{
    Q_OBJECT

private slots:
    void testRun_data()
    {
        QTest::addColumn<QString>("url");
        QTest::addRow("local") << QStringLiteral("local:replica");
        QTest::addRow("tcp") << QStringLiteral("tcp://127.0.0.1:65217");
    }

    void testRun()
    {
        QFETCH(QString, url);

        QProcess serverProc;
        serverProc.setProcessChannelMode(QProcess::ForwardedChannels);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("RO_URL", url);
        serverProc.setProcessEnvironment(env);
        const QString appDir = QCoreApplication::applicationDirPath();
        serverProc.start(findExecutable("qtro_reconnect_server", appDir + QLatin1String("/../server/")), QStringList());
        QVERIFY(serverProc.waitForStarted());

        QProcess clientProc;
        clientProc.setProcessChannelMode(QProcess::ForwardedChannels);
        clientProc.setProcessEnvironment(env);
        clientProc.start(findExecutable("qtro_reconnect_client", appDir + QLatin1String("/../client/")), QStringList());
        qDebug() << "Started server and client process on:" << url;
        QVERIFY(clientProc.waitForStarted());

        QVERIFY(clientProc.waitForFinished());
        QVERIFY(serverProc.waitForFinished());

        QCOMPARE(serverProc.exitCode(), 0);
        QCOMPARE(clientProc.exitCode(), 0);
    }
};

QTEST_MAIN(tst_Reconnect)

#include "tst_reconnect.moc"
