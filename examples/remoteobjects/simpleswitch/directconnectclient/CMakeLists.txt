cmake_minimum_required(VERSION 3.16)
project(directconnectclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/simpleswitch/directconnectclient")

find_package(Qt6 REQUIRED COMPONENTS Core RemoteObjects)

qt_add_executable(directconnectclient
    client.cpp client.h
    main.cpp
)

set_target_properties(directconnectclient PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(directconnectclient PUBLIC
    Qt::Core
    Qt::RemoteObjects
)

qt6_add_repc_replicas(directconnectclient
    simpleswitch.rep
)

install(TARGETS directconnectclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
