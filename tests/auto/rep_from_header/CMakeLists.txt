# Generated from pods.pro.

#####################################################################
## tst_pods Test:
#####################################################################

qt_add_test(tst_rep_from_header
    SOURCES
        tst_rep_from_header.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_rep_from_header(tst_rep_from_header pods.h)

#### Keys ignored in scope 1:.:.:pods.pro:<TRUE>:
# QOBJECT_REP = "$$REP_FILES"

## Scopes:
#####################################################################
