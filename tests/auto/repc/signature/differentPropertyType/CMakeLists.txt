# Generated from differentPropertyType.pro.

#####################################################################
## differentPropertyType Binary:
#####################################################################

add_qt_executable(differentPropertyType
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../mismatch.cpp
    INCLUDE_DIRECTORIES
        ..
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_replica(differentPropertyType
    mismatch.rep
)
