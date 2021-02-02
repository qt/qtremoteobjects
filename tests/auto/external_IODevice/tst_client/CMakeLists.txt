# Generated from tst_client.pro.

#####################################################################
## tst_external_IODevice Test:
#####################################################################

qt_internal_add_test(tst_external_IODevice
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_client.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
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

qt_internal_add_resource(tst_external_IODevice "cert"
    PREFIX
        "/sslcert"
    BASE
        "../cert"
    FILES
        ${cert_resource_files}
)

qt6_add_repc_replica(tst_external_IODevice
    ../pingpong.rep
)
