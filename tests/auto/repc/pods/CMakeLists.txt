# Generated from pods.pro.

#####################################################################
## tst_repc_pods Test:
#####################################################################

add_qt_test(tst_repc_pods
    SOURCES
        tst_pods.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_replica(tst_repc_pods
    pods.rep
)

## Scopes:
#####################################################################
