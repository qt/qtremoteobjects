# Generated from modelview.pro.

#####################################################################
## tst_modelview Test:
#####################################################################

add_qt_test(tst_modelview
    SOURCES
        modeltest.cpp modeltest.h
        tst_modelview.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::RemoteObjects
)

## Scopes:
#####################################################################

extend_target(tst_modelview CONDITION boot2qt
    DEFINES
        SLOW_MODELTEST
)

extend_target(tst_modelview CONDITION mingw
    DEFINES
        SLOW_MODELTEST
)
