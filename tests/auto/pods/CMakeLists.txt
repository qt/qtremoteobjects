# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_pods Test:
#####################################################################

qt_internal_add_test(tst_pods
    SOURCES
        tst_pods.cpp
    LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_replicas(tst_pods
    pods.h
)

## Scopes:
#####################################################################
