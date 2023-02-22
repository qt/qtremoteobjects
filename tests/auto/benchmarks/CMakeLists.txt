# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_benchmarkstest Test:
#####################################################################

qt_internal_add_test(tst_benchmarkstest
    SOURCES
        tst_benchmarkstest.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/"
    LIBRARIES
        Qt::Network
        Qt::RemoteObjects
)
qt6_add_repc_sources(tst_benchmarkstest
    ../repfiles/localdatacenter.rep
    ../repfiles/tcpdatacenter.rep
)
qt6_add_repc_replicas(tst_benchmarkstest
        ../repfiles/localdatacenter.rep
        ../repfiles/tcpdatacenter.rep
)
