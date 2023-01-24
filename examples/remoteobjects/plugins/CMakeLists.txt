# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qmlqrotimeexampleplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/plugins/imports/TimeExample")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml RemoteObjects)

qt6_add_qml_module(qmlqrotimeexampleplugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/imports/TimeExample"
    VERSION 1.0
    URI "TimeExample"
    RESOURCE_PREFIX /
)

target_sources(qmlqrotimeexampleplugin PRIVATE
    plugin.cpp
)

set_target_properties(qmlqrotimeexampleplugin PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(qmlqrotimeexampleplugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::RemoteObjects
)

qt6_add_repc_replicas(qmlqrotimeexampleplugin
    ../timemodel.rep
)

install(TARGETS qmlqrotimeexampleplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
