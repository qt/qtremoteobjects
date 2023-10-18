# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(tst_bleserver VERSION "${QT_REPO_MODULE_VERSION}" LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

qt_internal_add_manual_test(tst_bleserver
    SOURCES
        ../common/bleiodevice.cpp ../common/bleiodevice.h
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Bluetooth
        Qt::RemoteObjects
)

set_target_properties(tst_bleserver PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(tst_bleserver PUBLIC
    ../common
)

qt_add_repc_sources(tst_bleserver
    ../common/heater.rep
)

if (APPLE)
    # Using absolute path for shared plist files is a Ninja bug workaround
    get_filename_component(SHARED_PLIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common ABSOLUTE)
    if (IOS)
        set_target_properties(tst_bleserver PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.ios.plist"
        )
    else()
        set_target_properties(tst_bleserver PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.cmake.macos.plist"
        )
    endif()
endif()
