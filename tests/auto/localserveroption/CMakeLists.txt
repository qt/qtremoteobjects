# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
