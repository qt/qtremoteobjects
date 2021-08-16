# Generated from clientapp.pro.

cmake_minimum_required(VERSION 3.16)
project(clientapp LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/clientapp")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS RemoteObjects)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(clientapp
    main.cpp
)
set_target_properties(clientapp PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(clientapp PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::RemoteObjects
)


# Resources:
set(clientapp_resource_files
    "qml/plugins.qml"
    "qml/plugins0.qml"
    "qml/plugins1.qml"
    "qml/plugins2.qml"
)

qt6_add_resources(clientapp "clientapp"
    PREFIX
        "/qml"
    FILES
        ${clientapp_resource_files}
)

install(TARGETS clientapp
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
