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

#include <QtTest/QtTest>
#include <QMetaType>
#include <QProcess>

namespace {

QString findExecutable(const QString &executableName, const QStringList &pathHints)
{
    foreach (const auto &pathHint, pathHints) {
#ifdef Q_OS_WIN
        const QString executablePath = pathHint + executableName + ".exe";
#else
        const QString executablePath = pathHint + executableName;
#endif
        if (QFileInfo::exists(executablePath))
            return executablePath;
    }

    qWarning() << "Could not find executable:" << executableName << "in any of" << pathHints;
    return QString();
}

}
typedef QLatin1String _;
class tst_Signature: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QLoggingCategory::setFilterRules("qt.remoteobjects.warning=false");
    }

    void cleanup()
    {
        // wait for delivery of RemoveObject events to the source
        QTest::qWait(200);
    }

    void testRun()
    {
        qDebug() << "Starting signatureServer process";
        QProcess serverProc;
        serverProc.setProcessChannelMode(QProcess::ForwardedChannels);
        serverProc.start(findExecutable("signatureServer", {
            QCoreApplication::applicationDirPath() + "/../signatureServer/"
        }));
        QVERIFY(serverProc.waitForStarted());

        // wait for server start
        QTest::qWait(200);

        QStringList tests;
        tests << _("differentGlobalEnum")
              << _("differentClassEnum")
              << _("differentPropertyCount")
              << _("differentPropertyType")
              << _("scrambledProperties")
              << _("differentSlotCount")
              << _("differentSlotType")
              << _("differentSlotParamCount")
              << _("differentSlotParamType")
              << _("scrambledSlots")
              << _("differentSignalCount")
              << _("differentSignalParamCount")
              << _("differentSignalParamType")
              << _("scrambledSignals")
              << _("state")
              << _("matchAndQuit"); // matchAndQuit should be the last one

        foreach (auto test, tests) {
            qDebug() << "Starting" << test << "process";
            QProcess testProc;
            testProc.setProcessChannelMode(QProcess::ForwardedChannels);
            testProc.start(findExecutable(test, {
                QCoreApplication::applicationDirPath() + _("/../") + test + _("/")
            }));
            QVERIFY(testProc.waitForStarted());
            QVERIFY(testProc.waitForFinished());
            QCOMPARE(testProc.exitCode(), 0);
        }

        QVERIFY(serverProc.waitForFinished());
        QCOMPARE(serverProc.exitCode(), 0);
    }
};

QTEST_MAIN(tst_Signature)

#include "tst_signature.moc"
