# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_enums Test:
#####################################################################

qt_internal_add_test(tst_enums
    SOURCES
        tst_enums.cpp
    LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_replicas(tst_enums
    enums.rep
)

## Scopes:
#####################################################################
