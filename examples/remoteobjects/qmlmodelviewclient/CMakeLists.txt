# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qmlmodelviewclient LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/qmlmodelviewclient")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick RemoteObjects)

qt_add_executable(qmlmodelviewclient
    main.cpp
)

set_target_properties(qmlmodelviewclient PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(qmlmodelviewclient PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::RemoteObjects
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt6_add_resources(qmlmodelviewclient "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS qmlmodelviewclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
