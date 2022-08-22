# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## tst_integration Test:
#####################################################################

qt_internal_add_test(tst_integration
    SOURCES
        engine.cpp engine.h
        speedometer.cpp speedometer.h
        temperature.h
        tst_integration.cpp
    LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_sources(tst_integration
    engine.rep
    ../repfiles/localdatacenter.rep
    ../repfiles/tcpdatacenter.rep
)
qt6_add_repc_replicas(tst_integration
        engine.rep
        ../repfiles/localdatacenter.rep
        ../repfiles/tcpdatacenter.rep
)
qt6_add_repc_merged(tst_integration
            speedometer.rep
            enum.rep
            pod.rep
)

add_dependencies(tst_integration localsockettestserver)
