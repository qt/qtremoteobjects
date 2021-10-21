qt_internal_add_test(tst_reconnect
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES tst_reconnect.cpp
    PUBLIC_LIBRARIES Qt::RemoteObjects
)
