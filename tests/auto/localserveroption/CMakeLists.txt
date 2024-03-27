# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_localserveroption Test:
#####################################################################

qt_internal_add_test(tst_localserveroption
    SOURCES
        tst_localserveroption.cpp
    LIBRARIES
        Qt::RemoteObjects
        Qt::Core
)
