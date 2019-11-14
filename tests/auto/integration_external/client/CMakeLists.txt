# Generated from client.pro.

#####################################################################
## integration_external_client Binary:
#####################################################################

qt_add_executable(integration_external_client
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_replica(integration_external_client
    ../MyInterface.rep
)

#### Keys ignored in scope 1:.:.:client.pro:<TRUE>:
# TEMPLATE = "app"
