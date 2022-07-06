# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## tst_repcodegenerator Test:
#####################################################################

qt_internal_add_test(tst_repcodegenerator
    SOURCES
        tst_repcodegenerator.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_repcodegenerator
    classwithsignalonlytest.rep
    preprocessortest.rep
    classwithreadonlypropertytest.rep
)

## Scopes:
#####################################################################
