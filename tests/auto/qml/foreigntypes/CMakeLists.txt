# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_foreigntypes LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_foreigntypes
    SOURCES
        tst_foreigntypes.cpp
    LIBRARIES
        Qt::Qml
        Qt::RemoteObjects
)

qt6_add_repc_replicas(tst_foreigntypes
    foreigntypes.rep
)

qt_add_qml_module(tst_foreigntypes
    URI ForeignTypes
    VERSION 1.0
    SOURCES
        foreignreplica.h
)

if(NOT CMAKE_CROSSCOMPILING)
    target_compile_definitions(tst_foreigntypes PRIVATE
        QMLTYPES_FILE="$<TARGET_PROPERTY:tst_foreigntypes,QT_QML_MODULE_PLUGIN_TYPES_FILE>"
    )
endif()
