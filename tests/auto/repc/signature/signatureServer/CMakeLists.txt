
#####################################################################
## signatureServer Binary:
#####################################################################

qt_internal_add_executable(signatureServer
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/rep_server_source.h
        main.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_sources(signatureServer
    ../server.rep
)

#### Keys ignored in scope 1:.:.:signatureServer.pro:<TRUE>:
# TEMPLATE = "app"
