# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(tst_qmlmodelviewclient VERSION "${QT_REPO_MODULE_VERSION}" LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

qt_internal_add_manual_test(tst_qmlmodelviewclient
    SOURCES
        main.cpp
    LIBRARIES
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::RemoteObjects
)

set_target_properties(tst_qmlmodelviewclient PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt6_add_resources(tst_qmlmodelviewclient "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
