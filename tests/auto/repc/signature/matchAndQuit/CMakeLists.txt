
#####################################################################
## matchAndQuit Binary:
#####################################################################

qt_internal_add_executable(matchAndQuit
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_replicas(matchAndQuit
    ../server.rep
)

#### Keys ignored in scope 1:.:.:matchAndQuit.pro:<TRUE>:
# TEMPLATE = "app"
