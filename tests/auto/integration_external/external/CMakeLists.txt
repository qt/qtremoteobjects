
#####################################################################
## tst_integration_external Test:
#####################################################################

qt_internal_add_test(tst_integration_external
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_integration_external.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
)
