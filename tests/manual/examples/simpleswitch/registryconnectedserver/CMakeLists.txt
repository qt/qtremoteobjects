# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(tst_registryconnectedserver VERSION "${QT_REPO_MODULE_VERSION}" LANGUAGES CXX)

if (ANDROID)
    message(FATAL_ERROR "This project cannot be built on Android.")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

qt_internal_add_manual_test(tst_registryconnectedserver
    SOURCES
        main.cpp
        simpleswitch.cpp
        simpleswitch.h
    LIBRARIES
        Qt::Core
        Qt::RemoteObjects
)

set_target_properties(tst_registryconnectedserver PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

qt6_add_repc_sources(tst_registryconnectedserver
    simpleswitch.rep
)
