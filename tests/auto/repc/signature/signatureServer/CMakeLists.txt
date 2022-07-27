# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## signatureServer Binary:
#####################################################################

qt_internal_add_executable(signatureServer
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/rep_server_source.h
        main.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_sources(signatureServer
    ../server.rep
)

#### Keys ignored in scope 1:.:.:signatureServer.pro:<TRUE>:
# TEMPLATE = "app"
