// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 200
    height: 400
    property int counter: 0;
    MouseArea {
        anchors.fill: parent
        onClicked:
        {
            counter = (counter + 1) % 3;
            console.log(counter);
            pageLoader.source = "plugins"+counter+".qml"
        }
    }
    Loader {
        id: pageLoader
        source: "plugins0.qml"
    }
}
//![0]
