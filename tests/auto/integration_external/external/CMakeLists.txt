# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## tst_integration_external Test:
#####################################################################

qt_internal_add_test(tst_integration_external
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_integration_external.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
