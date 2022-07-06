# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## tst_modelreplicatest Test:
#####################################################################

qt_internal_add_test(tst_modelreplicatest
    SOURCES
        tst_modelreplicatest.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_modelreplicatest
    model.rep
)

#### Keys ignored in scope 1:.:.:modelreplica.pro:<TRUE>:
# TEMPLATE = "app"
