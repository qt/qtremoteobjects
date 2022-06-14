// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rep_preprocessortest_merged.h"

#include <QTest>

class tst_RepCodeGenerator : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testPreprocessorTestFile();
};

void tst_RepCodeGenerator::testPreprocessorTestFile()
{
    // could be a compile-time test, but let's just do something here
    QVERIFY(PREPROCESSORTEST_MACRO);
}

QTEST_APPLESS_MAIN(tst_RepCodeGenerator)

#include "tst_repcodegenerator.moc"
