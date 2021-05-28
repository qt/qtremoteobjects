
#####################################################################
## sslTestServer Binary:
#####################################################################

qt_internal_add_executable(sslTestServer
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
        pingpong.cpp pingpong.h
        sslserver.cpp sslserver.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)

# Resources:
set(cert_resource_files
    "../cert/client.crt"
    "../cert/client.key"
    "../cert/rootCA.key"
    "../cert/rootCA.pem"
    "../cert/rootCA.srl"
    "../cert/server.crt"
    "../cert/server.key"
)

qt_internal_add_resource(sslTestServer "cert"
    PREFIX
        "/sslcert"
    BASE
        "../cert"
    FILES
        ${cert_resource_files}
)

qt6_add_repc_sources(sslTestServer
    ../pingpong.rep
)

#### Keys ignored in scope 1:.:.:sslTestServer.pro:<TRUE>:
# TEMPLATE = "app"
