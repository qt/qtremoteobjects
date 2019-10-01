# Generated from pods.pro.

#####################################################################
## tst_pods Test:
#####################################################################

add_qt_test(tst_pods
    SOURCES
        tst_pods.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_replica(tst_pods
    pods.h
)

#### Keys ignored in scope 1:.:.:pods.pro:<TRUE>:
# QOBJECT_REP = "$$REP_FILES"

## Scopes:
#####################################################################
