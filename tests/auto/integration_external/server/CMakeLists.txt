# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## integration_external_server Binary:
#####################################################################

qt_internal_add_executable(integration_external_server
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
        mytestserver.cpp mytestserver.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_sources(integration_external_server
    ../MyInterface.rep
)

#### Keys ignored in scope 1:.:.:server.pro:<TRUE>:
# TEMPLATE = "app"
