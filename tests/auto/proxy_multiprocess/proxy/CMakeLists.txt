
#####################################################################
## proxy Binary:
#####################################################################

qt_internal_add_executable(proxy
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)

#### Keys ignored in scope 1:.:.:proxy.pro:<TRUE>:
# TEMPLATE = "app"
