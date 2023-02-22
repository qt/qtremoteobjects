# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_modelreplicatest Test:
#####################################################################

qt_internal_add_test(tst_modelreplicatest
    SOURCES
        tst_modelreplicatest.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/"
    LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_modelreplicatest
    model.rep
)
