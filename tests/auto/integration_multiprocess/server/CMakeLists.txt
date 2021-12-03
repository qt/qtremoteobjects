
#####################################################################
## integration_multiprocess_server Binary:
#####################################################################

qt_internal_add_executable(integration_multiprocess_server
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
        mytestserver.cpp mytestserver.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_merged(integration_multiprocess_server
    ../ExtPodInterface.rep
)

qt6_add_repc_sources(integration_multiprocess_server
    ../MyInterface.rep
    ../PodInterface.rep
)

#### Keys ignored in scope 1:.:.:server.pro:<TRUE>:
# TEMPLATE = "app"
