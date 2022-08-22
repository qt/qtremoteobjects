# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_integration_external Test:
#####################################################################

qt_internal_add_test(tst_integration_external
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_integration_external.cpp
    LIBRARIES
        Qt::RemoteObjects
)
