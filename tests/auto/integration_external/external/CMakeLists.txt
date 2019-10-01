# Generated from external.pro.

#####################################################################
## tst_integration_external Test:
#####################################################################

add_qt_test(tst_integration_external
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_integration_external.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
