# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_repcodegenerator Test:
#####################################################################

qt_internal_add_test(tst_repcodegenerator
    SOURCES
        tst_repcodegenerator.cpp
    LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_repcodegenerator
    classwithsignalonlytest.rep
    preprocessortest.rep
    classwithreadonlypropertytest.rep
)

## Scopes:
#####################################################################
