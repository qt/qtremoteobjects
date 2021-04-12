
#####################################################################
## tst_usertypes Test:
#####################################################################

qt_internal_add_test(tst_usertypes
    SOURCES
        tst_usertypes.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_usertypes
    usertypes.rep
)

#### Keys ignored in scope 1:.:.:usertypes.pro:<TRUE>:
# TEMPLATE = "app"
