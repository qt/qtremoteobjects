qt_internal_add_executable(qtro_reconnect_server
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES main.cpp
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES Qt::RemoteObjects Qt::Test
)
