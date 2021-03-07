# Generated from qt_source.pro.

cmake_minimum_required(VERSION 3.14)
project(QtSource LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/remoteobjects/cbor_python/qt_source")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS RemoteObjects)

qt_add_executable(QtSource
    main.cpp
    simple.cpp simple.h
)
set_target_properties(QtSource PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)
target_link_libraries(QtSource PUBLIC
    Qt::Core
    Qt::Gui
    Qt::RemoteObjects
)

qt6_add_repc_sources(QtSource
    Simple.rep
)

install(TARGETS QtSource
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# special case begin
add_custom_command(
        TARGET QtSource POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/../py_replica/py_replica.py
                $<TARGET_FILE_DIR:QtSource>/../py_replica/py_replica.py
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/Simple.rep
                $<TARGET_FILE_DIR:QtSource>/Simple.rep
    )
install(FILES ../py_replica/py_replica.py DESTINATION "${INSTALL_EXAMPLEDIR}/../py_replica")
install(FILES Simple.rep DESTINATION "${INSTALL_EXAMPLEDIR}")
# special case end
