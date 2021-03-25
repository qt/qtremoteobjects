
#####################################################################
## tst_proxy Test:
#####################################################################

qt_internal_add_test(tst_proxy
    SOURCES
        tst_proxy.cpp
        ../shared/model_utilities.h
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_proxy
    engine.rep
    subclass.rep
)

## Scopes:
#####################################################################
