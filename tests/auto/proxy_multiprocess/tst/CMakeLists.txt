# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_proxy_multiprocess Test:
#####################################################################

qt_internal_add_test(tst_proxy_multiprocess
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_proxy_multiprocess.cpp
    LIBRARIES
        Qt::RemoteObjects
)
