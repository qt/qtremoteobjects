
#####################################################################
## localsockettestserver Binary:
#####################################################################

qt_internal_add_executable(localsockettestserver
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
    OUTPUT_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}
)

#### Keys ignored in scope 1:.:.:localsockettestserver.pro:<TRUE>:
# TEMPLATE = "app"
