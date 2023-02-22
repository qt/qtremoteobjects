# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


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
