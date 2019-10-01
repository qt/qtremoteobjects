# Generated from scrambledProperties.pro.

#####################################################################
## scrambledProperties Binary:
#####################################################################

add_qt_executable(scrambledProperties
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../mismatch.cpp
    INCLUDE_DIRECTORIES
        ..
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_replica(scrambledProperties
    mismatch.rep
)
