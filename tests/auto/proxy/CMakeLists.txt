# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## tst_proxy Test:
#####################################################################

qt_internal_add_test(tst_proxy
    SOURCES
        tst_proxy.cpp
        ../shared/model_utilities.h
    LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_proxy
    engine.rep
    subclass.rep
)

## Scopes:
#####################################################################
