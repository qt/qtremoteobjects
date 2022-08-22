# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_restart Test:
#####################################################################

qt_internal_add_test(tst_restart
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_restart.cpp
    LIBRARIES
        Qt::RemoteObjects
)
