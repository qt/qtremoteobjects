# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET Qt::Quick AND UNIX AND NOT ANDROID)
    add_subdirectory(qmlmodelviewclient)
endif()
add_subdirectory(simpleswitch)
if(TARGET Qt::Bluetooth AND TARGET Qt::Widgets)
    add_subdirectory(ble)
endif()
