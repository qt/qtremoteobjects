import QtQuick 2.0
import QtRemoteObjects 5.15
import usertypes 1.0

Item {
    SimpleClockSimpleSource {
        id: clock
    }

    Host {
        hostUrl: "local:testHost"
        Component.onCompleted: enableRemoting(clock)
    }
}
