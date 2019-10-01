# Generated from sslcppclient.pro.

cmake_minimum_required(VERSION 3.14)
project(SslCppClient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/ssl/sslcppclient")

find_package(Qt6 COMPONENTS RemoteObjects)
find_package(Qt6 COMPONENTS Core)

add_qt_gui_executable(SslCppClient
    main.cpp
)
target_link_libraries(SslCppClient PRIVATE
    Qt::RemoteObjectsPrivate
)

target_link_libraries(SslCppClient PUBLIC
    # Remove: gui
    Qt::Core
    Qt::RemoteObjects
)


# Resources:
set_source_files_properties("../sslserver/cert/client.crt"
    PROPERTIES QT_RESOURCE_ALIAS "client.crt"
)
set_source_files_properties("../sslserver/cert/client.key"
    PROPERTIES QT_RESOURCE_ALIAS "client.key"
)
set_source_files_properties("../sslserver/cert/rootCA.key"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.key"
)
set_source_files_properties("../sslserver/cert/rootCA.pem"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.pem"
)
set_source_files_properties("../sslserver/cert/rootCA.srl"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.srl"
)
set_source_files_properties("../sslserver/cert/server.crt"
    PROPERTIES QT_RESOURCE_ALIAS "server.crt"
)
set_source_files_properties("../sslserver/cert/server.key"
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

qt6_add_resources(SslCppClient "cert"
    PREFIX
        "/sslcert"
    BASE
        "../sslserver/cert"
    FILES
        ${cert_resource_files}
)

qt6_add_repc_replica(SslCppClient
    timemodel.rep
)

install(TARGETS SslCppClient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
