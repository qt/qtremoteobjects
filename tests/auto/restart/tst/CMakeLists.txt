
#####################################################################
## tst_restart Test:
#####################################################################

qt_internal_add_test(tst_restart
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_restart.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
