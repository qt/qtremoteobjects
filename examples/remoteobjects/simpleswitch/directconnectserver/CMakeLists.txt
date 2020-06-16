# Generated from directconnectserver.pro.

cmake_minimum_required(VERSION 3.14)
project(directconnectserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/simpleswitch/directconnectserver")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS RemoteObjects)

add_executable(directconnectserver
    main.cpp
    simpleswitch.cpp simpleswitch.h
)
target_link_libraries(directconnectserver PUBLIC
    Qt::Core
    Qt::RemoteObjects
)

qt6_add_repc_source(directconnectserver
    simpleswitch.rep
)

install(TARGETS directconnectserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
