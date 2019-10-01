# Generated from directconnectclient.pro.

cmake_minimum_required(VERSION 3.14)
project(directconnectclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/simpleswitch/directconnectclient")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS RemoteObjects)

add_executable(directconnectclient
    client.cpp client.h
    main.cpp
)
target_link_libraries(directconnectclient PUBLIC
    Qt::Core
    Qt::RemoteObjects
)

qt6_add_repc_replica(directconnectclient
    simpleswitch.rep
)

install(TARGETS directconnectclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
