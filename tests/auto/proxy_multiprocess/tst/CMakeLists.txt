# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


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
