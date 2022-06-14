// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui/QGuiApplication>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickView>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickView viewer;
    viewer.engine()->addImportPath(QStringLiteral("qrc:/qml"));
    viewer.setSource(QUrl(QStringLiteral("qrc:/qml/qml/plugins.qml")));
    viewer.show();

    return app.exec();
}
