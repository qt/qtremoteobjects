# Generated from enums.pro.

#####################################################################
## tst_enums Test:
#####################################################################

add_qt_test(tst_enums
    SOURCES
        tst_enums.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_replica(tst_enums
    enums.rep
)

## Scopes:
#####################################################################
