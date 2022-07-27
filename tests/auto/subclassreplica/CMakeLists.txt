# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## tst_subclassreplicatest Test:
#####################################################################

qt_internal_add_test(tst_subclassreplicatest
    SOURCES
        tst_subclassreplicatest.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_subclassreplicatest
    class.rep
)

#### Keys ignored in scope 1:.:.:subclassreplica.pro:<TRUE>:
# TEMPLATE = "app"
