// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick
import TimeExample // import types from the plugin

Rectangle {
    width: 200
    height: 400
    color: "blue"

    Time { // this class is defined in C++ (plugin.cpp)
        id: time
    }

    Text {
        id: validity; font.bold: true; font.pixelSize: 14; y:200; color: "white"
        anchors.horizontalCenter: parent.horizontalCenter
        text: time.isValid ? "Click to see clock!" : "Server not found"
    }
}
//![0]
