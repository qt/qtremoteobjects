# Generated from cppclient.pro.

cmake_minimum_required(VERSION 3.14)
project(CppClient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/cppclient")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS RemoteObjects)

add_qt_gui_executable(CppClient
    main.cpp
)
target_link_libraries(CppClient PUBLIC
    Qt::Core
    Qt::RemoteObjects
)

qt6_add_repc_replica(CppClient
    timemodel.rep
)

install(TARGETS CppClient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
