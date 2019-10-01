# Generated from integration.pro.

#####################################################################
## tst_integration Test:
#####################################################################

add_qt_test(tst_integration
    SOURCES
        engine.cpp engine.h
        speedometer.cpp speedometer.h
        temperature.h
        tst_integration.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_source(tst_integration
    engine.rep
    ../repfiles/localdatacenter.rep
    ../repfiles/tcpdatacenter.rep
)
qt6_add_repc_replica(tst_integration
        engine.rep
        ../repfiles/localdatacenter.rep
        ../repfiles/tcpdatacenter.rep
)
qt6_add_repc_merged(tst_integration
            speedometer.rep
            enum.rep
            pod.rep
)

# special case begin
add_dependencies(tst_integration localsockettestserver)
# special case end

## Scopes:
#####################################################################
