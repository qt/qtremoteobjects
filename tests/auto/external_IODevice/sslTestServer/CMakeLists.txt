
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
set_source_files_properties("../cert/client.crt"
    PROPERTIES QT_RESOURCE_ALIAS "client.crt"
)
set_source_files_properties("../cert/client.key"
    PROPERTIES QT_RESOURCE_ALIAS "client.key"
)
set_source_files_properties("../cert/rootCA.key"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.key"
)
set_source_files_properties("../cert/rootCA.pem"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.pem"
)
set_source_files_properties("../cert/rootCA.srl"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.srl"
)
set_source_files_properties("../cert/server.crt"
    PROPERTIES QT_RESOURCE_ALIAS "server.crt"
)
set_source_files_properties("../cert/server.key"
    PROPERTIES QT_RESOURCE_ALIAS "server.key"
)
set(cert_resource_files
    "client.crt"
    "client.key"
    "rootCA.key"
    "rootCA.pem"
    "rootCA.srl"
    "server.crt"
    "server.key"
)

qt_internal_add_resource(sslTestServer "cert"
    PREFIX
        "/sslcert"
    BASE
        "../cert"
    FILES
        ${cert_resource_files}
)

qt6_add_repc_source(sslTestServer
    ../pingpong.rep
)

#### Keys ignored in scope 1:.:.:sslTestServer.pro:<TRUE>:
# TEMPLATE = "app"
