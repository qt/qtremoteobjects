# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(SslCppClient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/ssl/sslcppclient")

find_package(Qt6 REQUIRED COMPONENTS Core RemoteObjects)

qt_add_executable(SslCppClient
    main.cpp
)

set_target_properties(SslCppClient PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(SslCppClient PUBLIC
    # Remove: gui
    Qt::Core
    Qt::RemoteObjects
    Qt::RemoteObjectsPrivate
)

# Resources:
set(cert_resource_files
    "../sslserver/cert/client.crt"
    "../sslserver/cert/client.key"
    "../sslserver/cert/rootCA.key"
    "../sslserver/cert/rootCA.pem"
    "../sslserver/cert/rootCA.srl"
    "../sslserver/cert/server.crt"
    "../sslserver/cert/server.key"
)

qt6_add_resources(SslCppClient "cert"
    PREFIX
        "/sslcert"
    BASE
        "../sslserver/cert"
    FILES
        ${cert_resource_files}
)

qt6_add_repc_replicas(SslCppClient
    timemodel.rep
)

install(TARGETS SslCppClient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
