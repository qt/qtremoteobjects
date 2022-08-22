# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_modelview Test:
#####################################################################

qt_internal_add_test(tst_modelview
    SOURCES
        tst_modelview.cpp
        ../shared/model_utilities.h
    LIBRARIES
        Qt::Gui
        Qt::RemoteObjects
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_modelview CONDITION boot2qt
    DEFINES
        SLOW_MODELTEST
)

qt_internal_extend_target(tst_modelview CONDITION MINGW
    DEFINES
        SLOW_MODELTEST
)
