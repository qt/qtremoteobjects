cmake_minimum_required(VERSION 3.1.0)

project(directconnectserver)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

#! [remote_objects_cmake]
find_package(Qt6 COMPONENTS RemoteObjects REQUIRED)
#! [remote_objects_cmake]

set(SOURCES
    main.cpp
    simpleswitch.cpp
)

#! [simpleSwitch_cmake_add_repc_source]
qt6_add_repc_source(directconnectserver
    simpleswitch.rep
)
#! [simpleSwitch_cmake_add_repc_source]

#! [simpleSwitch_cmake_add_repc_replica]
qt6_add_repc_replica(directconnectclient
    simpleswitch.rep
)
#! [simpleSwitch_cmake_add_repc_replica]

#! [simpleSwitch_cmake_add_repc_merged]
qt6_add_repc_merged(directconnectexample
    simpleswitch.rep
)
#! [simpleSwitch_cmake_add_repc_merged]

add_executable(directconnectserver ${SOURCES})
#! [remote_objects_cmake_link]
target_link_libraries(directconnectserver PRIVATE Qt6::RemoteObjects)
#! [remote_objects_cmake_link]
