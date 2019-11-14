# Generated from wsserver.pro.

cmake_minimum_required(VERSION 3.14)
project(wsserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/websockets/wsserver")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS RemoteObjects)
find_package(Qt6 COMPONENTS WebSockets)

add_executable(wsserver
    ../common/websocketiodevice.cpp ../common/websocketiodevice.h
    main.cpp
)
target_include_directories(wsserver PUBLIC
    ../common
)

target_link_libraries(wsserver PUBLIC
    Qt::Core
    Qt::Gui
    Qt::RemoteObjects
    Qt::WebSockets
    Qt::Widgets
)


# Resources:
set_source_files_properties("../common/cert/client.crt"
    PROPERTIES QT_RESOURCE_ALIAS "client.crt"
)
set_source_files_properties("../common/cert/client.key"
    PROPERTIES QT_RESOURCE_ALIAS "client.key"
)
set_source_files_properties("../common/cert/rootCA.key"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.key"
)
set_source_files_properties("../common/cert/rootCA.pem"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.pem"
)
set_source_files_properties("../common/cert/rootCA.srl"
    PROPERTIES QT_RESOURCE_ALIAS "rootCA.srl"
)
set_source_files_properties("../common/cert/server.crt"
    PROPERTIES QT_RESOURCE_ALIAS "server.crt"
)
set_source_files_properties("../common/cert/server.key"
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

qt6_add_resources(wsserver "cert"
    PREFIX
        "/sslcert"
    BASE
        "../common/cert"
    FILES
        ${cert_resource_files}
)

install(TARGETS wsserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
