// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qfile.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtTest/qtest.h>

#include "foreignreplica.h"

class tst_foreigntypes : public QObject
{
    Q_OBJECT

private slots:
    void qmltypes();
    void enumInQml();
};

void tst_foreigntypes::qmltypes()
{
#ifndef QMLTYPES_FILE
    QSKIP("The qmltypes file is not accessible on this platform.");
#else
    QFile file(QStringLiteral(QMLTYPES_FILE));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    const QByteArray content = file.readAll();
    // Without the metatypes of the generated header, qmltyperegistrar knows only the name.
    QVERIFY(content.contains("resolvedFile: \"rep_foreigntypes_replica.h\""));
    QVERIFY(content.contains("prototype: \"QRemoteObjectReplica\""));
    QVERIFY(content.contains("name: \"Mode\""));
#endif
}

void tst_foreigntypes::enumInQml()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQml\nimport ForeignTypes\n"
                      "QtObject { property int mode: Foreign.On }",
                      QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    QCOMPARE(object->property("mode").toInt(), int(ForeignObjectReplica::On));
}

QTEST_MAIN(tst_foreigntypes)

#include "tst_foreigntypes.moc"
