
#####################################################################
## restart_server Binary:
#####################################################################

qt_internal_add_executable(restart_server
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
qt6_add_repc_sources(restart_server
    ../subclass.rep
)

#### Keys ignored in scope 1:.:.:server.pro:<TRUE>:
# TEMPLATE = "app"
