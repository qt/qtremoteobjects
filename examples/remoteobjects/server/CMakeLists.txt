# Generated from server.pro.

cmake_minimum_required(VERSION 3.14)
project(server LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/server")

find_package(Qt6 COMPONENTS RemoteObjects)
find_package(Qt6 COMPONENTS Core)

qt_add_executable(server
    main.cpp
    timemodel.cpp timemodel.h
)
set_target_properties(server PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(server PUBLIC
    Qt::Core
    Qt::RemoteObjects
)

qt6_add_repc_source(server
    ../timemodel.rep
)

install(TARGETS server
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
