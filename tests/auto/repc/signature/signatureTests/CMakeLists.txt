# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_signature Test:
#####################################################################

qt_internal_add_test(tst_ro_signature
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_signature.cpp
    LIBRARIES
        Qt::RemoteObjects
)
