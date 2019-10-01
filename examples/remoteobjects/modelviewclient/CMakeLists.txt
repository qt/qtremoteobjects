# Generated from modelviewclient.pro.

cmake_minimum_required(VERSION 3.14)
project(modelviewclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/modelviewclient")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS RemoteObjects)

add_qt_gui_executable(modelviewclient
    main.cpp
)
target_link_libraries(modelviewclient PUBLIC
    Qt::Core
    Qt::Gui
    Qt::RemoteObjects
    Qt::Widgets
)

install(TARGETS modelviewclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
