
#####################################################################
## integration_multiprocess_client Binary:
#####################################################################

qt_internal_add_executable(integration_multiprocess_client
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)

qt6_add_repc_merged(integration_multiprocess_client
    ../ExtPodInterface.rep
)

qt6_add_repc_replicas(integration_multiprocess_client
    ../MyInterface.rep
)

#### Keys ignored in scope 1:.:.:client.pro:<TRUE>:
# TEMPLATE = "app"
