# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## tst_benchmarkstest Test:
#####################################################################

qt_internal_add_test(tst_benchmarkstest
    SOURCES
        tst_benchmarkstest.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    PUBLIC_LIBRARIES
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

#### Keys ignored in scope 1:.:.:benchmarks.pro:<TRUE>:
# TEMPLATE = "app"
