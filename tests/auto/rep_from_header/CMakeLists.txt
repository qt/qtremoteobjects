# Generated from rep_from_header.pro.

#####################################################################
## tst_rep_from_header Test:
#####################################################################

qt_internal_add_test(tst_rep_from_header
    SOURCES
        tst_rep_from_header.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
qt6_rep_from_header(tst_rep_from_header pods.h)

#### Keys ignored in scope 1:.:.:rep_from_header.pro:<TRUE>:
# QOBJECT_REP = "$$REP_FILES"
