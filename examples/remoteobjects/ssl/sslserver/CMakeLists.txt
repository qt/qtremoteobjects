# Generated from sslserver.pro.

cmake_minimum_required(VERSION 3.14)
project(sslserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/ssl/sslserver")

find_package(Qt6 COMPONENTS RemoteObjects)
find_package(Qt6 COMPONENTS Core)

add_executable(sslserver
    main.cpp
    sslserver.cpp sslserver.h
    timemodel.cpp timemodel.h
)
target_link_libraries(sslserver PRIVATE
    Qt::RemoteObjectsPrivate
)

target_link_libraries(sslserver PUBLIC
    Qt::Core
    Qt::RemoteObjects
)


# Resources:
set(cert_resource_files
    "client.crt"
    "client.key"
    "rootCA.key"
    "rootCA.pem"
    "rootCA.srl"
    "server.crt"
    "server.key"
)

qt6_add_resources(sslserver "cert"
    PREFIX
        "/sslcert"
    BASE
        "cert"
    FILES
        ${cert_resource_files}
)

qt6_add_repc_source(sslserver
    ../../timemodel.rep
)

install(TARGETS sslserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
