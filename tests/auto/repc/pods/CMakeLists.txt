# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## tst_repc_pods Test:
#####################################################################

qt_internal_add_test(tst_repc_pods
    SOURCES
        tst_pods.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_replicas(tst_repc_pods
    pods.rep
)

## Scopes:
#####################################################################
