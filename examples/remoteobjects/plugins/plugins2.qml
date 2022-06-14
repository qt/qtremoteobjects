// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick 2.0
import TimeExample 1.0 // import types from the plugin

Rectangle {
    width: 200
    height: 400
    Clock { // this class is defined in QML (imports/TimeExample/Clock.qml)
        id: clock1
        anchors.top: parent.top
        Time { // this class is defined in C++ (plugin.cpp)
            id: time
        }

        hours: time.hour
        minutes: time.minute

    }
    Clock { // this class is defined in QML (imports/TimeExample/Clock.qml)
        id: clock2
        anchors.top: clock1.bottom
        Time { // this class is defined in C++ (plugin.cpp)
            id: time2
        }

        hours: time2.hour
        minutes: time2.minute

    }

}
//![0]
