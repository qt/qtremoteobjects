
#####################################################################
## tst_integration_multiprocess Test:
#####################################################################

qt_internal_add_test(tst_integration_multiprocess
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_integration_multiprocess.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
