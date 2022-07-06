# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_executable(qtro_reconnect_server
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES main.cpp
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES Qt::RemoteObjects Qt::Test
)
