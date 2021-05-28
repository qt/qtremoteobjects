
#####################################################################
## tst_repc_pods Test:
#####################################################################

qt_internal_add_test(tst_repc_pods
    SOURCES
        tst_pods.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_replicas(tst_repc_pods
    pods.rep
)

## Scopes:
#####################################################################
