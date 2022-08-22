# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(registryconnectedserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/simpleswitch/registryconnectedserver")

find_package(Qt6 REQUIRED COMPONENTS Core RemoteObjects)

qt_add_executable(registryconnectedserver
    main.cpp
    simpleswitch.cpp simpleswitch.h
)

set_target_properties(registryconnectedserver PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(registryconnectedserver PUBLIC
    Qt::Core
    Qt::RemoteObjects
)

qt6_add_repc_sources(registryconnectedserver
    simpleswitch.rep
)

install(TARGETS registryconnectedserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
