# Generated from scrambledSignals.pro.

#####################################################################
## scrambledSignals Binary:
#####################################################################

add_qt_executable(scrambledSignals
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../mismatch.cpp
    INCLUDE_DIRECTORIES
        ..
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_replica(scrambledSignals
    mismatch.rep
)
