
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
set(cert_resource_files
    "../cert/client.crt"
    "../cert/client.key"
    "../cert/rootCA.key"
    "../cert/rootCA.pem"
    "../cert/rootCA.srl"
    "../cert/server.crt"
    "../cert/server.key"
)

qt_internal_add_resource(tst_external_IODevice "cert"
    PREFIX
        "/sslcert"
    BASE
        "../cert"
    FILES
        ${cert_resource_files}
)

qt6_add_repc_replicas(tst_external_IODevice
    ../pingpong.rep
)
