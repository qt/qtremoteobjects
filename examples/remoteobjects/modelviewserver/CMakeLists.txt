# Generated from modelviewserver.pro.

cmake_minimum_required(VERSION 3.14)
project(modelviewserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/remoteobjects/modelviewserver")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS RemoteObjects)

add_qt_gui_executable(modelviewserver
    main.cpp
)
target_link_libraries(modelviewserver PUBLIC
    Qt::Core
    Qt::Gui
    Qt::RemoteObjects
    Qt::Widgets
)

install(TARGETS modelviewserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
