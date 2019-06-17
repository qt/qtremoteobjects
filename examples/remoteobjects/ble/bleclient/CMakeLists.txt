# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(bleclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/ble/bleclient")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Bluetooth RemoteObjects)

qt_add_executable(bleclient
    ../common/bleiodevice.cpp ../common/bleiodevice.h
    connectpage.cpp connectpage.h connectpage.ui
    mainwindow.cpp mainwindow.h
    heaterview.cpp heaterview.h heaterview.ui
    main.cpp
)

set_target_properties(bleclient PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(bleclient PUBLIC
    ../common
)

target_link_libraries(bleclient PUBLIC
    Qt::Core
    Qt::Widgets
    Qt::Bluetooth
    Qt::RemoteObjects
)

qt6_add_repc_replicas(bleclient
    ../common/heater.rep
)

if (APPLE)
    # Using absolute path for shared plist files is a Ninja bug workaround
    get_filename_component(SHARED_PLIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common ABSOLUTE)
    if (IOS)
        set_target_properties(bleclient PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.ios.plist"
        )
    else()
        set_target_properties(bleclient PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.cmake.macos.plist"
        )
    endif()
endif()

install(TARGETS bleclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
