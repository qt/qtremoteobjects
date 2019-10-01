# Generated from tst.pro.

#####################################################################
## tst_integration_multiprocess Test:
#####################################################################

add_qt_test(tst_integration_multiprocess
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_integration_multiprocess.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
