# Generated from differentSignalParamCount.pro.

#####################################################################
## differentSignalParamCount Binary:
#####################################################################

qt_add_executable(differentSignalParamCount
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../mismatch.cpp
    INCLUDE_DIRECTORIES
        ..
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_replica(differentSignalParamCount
    mismatch.rep
)
