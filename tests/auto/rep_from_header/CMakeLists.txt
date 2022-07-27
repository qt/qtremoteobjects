# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## tst_rep_from_header Test:
#####################################################################

qt_internal_add_test(tst_rep_from_header
    SOURCES
        tst_rep_from_header.cpp
    LIBRARIES
        Qt::RemoteObjects
)
qt6_reps_from_headers(tst_rep_from_header pods.h)

#### Keys ignored in scope 1:.:.:rep_from_header.pro:<TRUE>:
# QOBJECT_REP = "$$REP_FILES"
