# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(bleserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/ble/bleserver")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Bluetooth RemoteObjects)

qt_add_executable(bleserver
    ../common/bleiodevice.cpp ../common/bleiodevice.h
    main.cpp
)

set_target_properties(bleserver PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(bleserver PUBLIC
    ../common
)

target_link_libraries(bleserver PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Bluetooth
    Qt::RemoteObjects
)

qt6_add_repc_sources(bleserver
    ../common/heater.rep
)

if (APPLE)
    # Using absolute path for shared plist files is a Ninja bug workaround
    get_filename_component(SHARED_PLIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common ABSOLUTE)
    if (IOS)
        set_target_properties(bleserver PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.ios.plist"
        )
    else()
        set_target_properties(bleserver PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.cmake.macos.plist"
        )
    endif()
endif()

install(TARGETS bleserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
