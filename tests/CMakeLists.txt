# Generated from tests.pro.

if(NOT TARGET Qt::Test)
    cmake_minimum_required(VERSION 3.15.0)
    project(QtRemoteObjectsTests VERSION 6.0.0 LANGUAGES C CXX)
    find_package(Qt6 ${PROJECT_VERSION} REQUIRED COMPONENTS BuildInternals Core RemoteObjects Test) # special case
    find_package(Qt6 ${PROJECT_VERSION} OPTIONAL_COMPONENTS Qml QuickTest) # special case
    qt_set_up_standalone_tests_build()
endif()
qt_build_tests()
