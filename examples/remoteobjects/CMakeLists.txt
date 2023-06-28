# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(remoteobjects_server)
add_subdirectory(simpleswitch)
add_subdirectory(websockets)
if(TARGET Qt::Widgets)
    qt_internal_add_example(modelviewclient)
    qt_internal_add_example(modelviewserver)
if(TARGET Qt::Bluetooth)
    add_subdirectory(ble)
endif()
endif()
if(QT_FEATURE_ssl)
    add_subdirectory(ssl)
endif()
if(TARGET Qt::Quick)
    qt_internal_add_example(clientapp)
endif()
