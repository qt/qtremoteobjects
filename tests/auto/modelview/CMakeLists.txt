
#####################################################################
## tst_modelview Test:
#####################################################################

qt_internal_add_test(tst_modelview
    SOURCES
        tst_modelview.cpp
        ../shared/model_utilities.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::RemoteObjects
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_modelview CONDITION boot2qt
    DEFINES
        SLOW_MODELTEST
)

qt_internal_extend_target(tst_modelview CONDITION MINGW
    DEFINES
        SLOW_MODELTEST
)
