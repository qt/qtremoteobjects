# Generated from registryconnectedclient.pro.

cmake_minimum_required(VERSION 3.14)
project(registryconnectedclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/simpleswitch/registryconnectedclient")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS RemoteObjects)

add_executable(registryconnectedclient
    dynamicclient.cpp dynamicclient.h
    main.cpp
)
target_link_libraries(registryconnectedclient PUBLIC
    Qt::Core
    Qt::RemoteObjects
)

install(TARGETS registryconnectedclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
