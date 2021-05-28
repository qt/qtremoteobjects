
#####################################################################
## differentClassEnum Binary:
#####################################################################

qt_internal_add_executable(differentClassEnum
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../mismatch.cpp
    INCLUDE_DIRECTORIES
        ..
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_replicas(differentClassEnum
    mismatch.rep
)
