
#####################################################################
## tst_repcodegenerator Test:
#####################################################################

qt_internal_add_test(tst_repcodegenerator
    SOURCES
        tst_repcodegenerator.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_add_repc_merged(tst_repcodegenerator
    classwithsignalonlytest.rep
    preprocessortest.rep
    classwithreadonlypropertytest.rep
)

## Scopes:
#####################################################################
