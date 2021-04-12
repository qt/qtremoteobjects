
#####################################################################
## tst_modelreplicatest Test:
#####################################################################

qt_internal_add_test(tst_modelreplicatest
    SOURCES
        tst_modelreplicatest.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_modelreplicatest
    model.rep
)

#### Keys ignored in scope 1:.:.:modelreplica.pro:<TRUE>:
# TEMPLATE = "app"
