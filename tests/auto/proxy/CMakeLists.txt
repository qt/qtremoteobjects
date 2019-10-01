# Generated from proxy.pro.

#####################################################################
## tst_proxy Test:
#####################################################################

add_qt_test(tst_proxy
    SOURCES
        tst_proxy.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_proxy
    engine.rep
    subclass.rep
)

## Scopes:
#####################################################################
