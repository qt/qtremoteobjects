# Generated from directconnectdynamicclient.pro.

cmake_minimum_required(VERSION 3.16)
project(directconnectdynamicclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/simpleswitch/directconnectdynamicclient")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS RemoteObjects)

qt_add_executable(directconnectdynamicclient
    dynamicclient.cpp dynamicclient.h
    main.cpp
)
set_target_properties(directconnectdynamicclient PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)
target_link_libraries(directconnectdynamicclient PUBLIC
    Qt::Core
    Qt::RemoteObjects
)

install(TARGETS directconnectdynamicclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
