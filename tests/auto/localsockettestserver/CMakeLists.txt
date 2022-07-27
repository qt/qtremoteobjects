# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## localsockettestserver Binary:
#####################################################################

qt_internal_add_executable(localsockettestserver
    SOURCES
        main.cpp
    LIBRARIES
        Qt::RemoteObjects
    OUTPUT_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}
)

#### Keys ignored in scope 1:.:.:localsockettestserver.pro:<TRUE>:
# TEMPLATE = "app"
