/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
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

#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <QDebug>
#include <QString>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

namespace TestUtils {

QString subFolder;
QString rootFolder;

bool init(const QString &folder)
{
    if (!TestUtils::rootFolder.isEmpty())
        return true;
    const auto appPath = QCoreApplication::applicationDirPath();
    auto dir = QDir(appPath);
    while (dir.dirName() != folder) {
        TestUtils::subFolder.prepend(dir.dirName()).prepend('/');
        if (!dir.cdUp())
            return false;
    }
    dir.cdUp();
    TestUtils::rootFolder = dir.absolutePath();
    return true;
}

QString findExecutable(const QString &executableName, const QString &folder)
{
    const auto path = QStandardPaths::findExecutable(executableName, {TestUtils::rootFolder + folder + TestUtils::subFolder});
    if (!path.isEmpty()) {
        return path;
    }

    qWarning() << "Could not find executable:" << executableName << "in " << TestUtils::rootFolder + folder + TestUtils::subFolder;
    return QString();
}

}

#endif // TESTUTILS_H
