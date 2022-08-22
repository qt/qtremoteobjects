# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(sslserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/ssl/sslserver")

find_package(Qt6 REQUIRED COMPONENTS Core RemoteObjects)

qt_add_executable(sslserver
    main.cpp
    sslserver.cpp sslserver.h
    timemodel.cpp timemodel.h
)

set_target_properties(sslserver PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(sslserver PUBLIC
    Qt::Core
    Qt::RemoteObjects
    Qt::RemoteObjectsPrivate
)

# Resources:
set(cert_resource_files
    "cert/client.crt"
    "cert/client.key"
    "cert/rootCA.key"
    "cert/rootCA.pem"
    "cert/rootCA.srl"
    "cert/server.crt"
    "cert/server.key"
)

qt6_add_resources(sslserver "cert"
    PREFIX
        "/sslcert"
    BASE
        "cert"
    FILES
        ${cert_resource_files}
)

qt6_add_repc_sources(sslserver
    ../../timemodel.rep
)

install(TARGETS sslserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
