
#####################################################################
## proxy_multiprocess_client Binary:
#####################################################################

qt_internal_add_executable(proxy_multiprocess_client
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../namespace.h
        ../shared.h
        main.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::RemoteObjects
        Qt::Test
)
qt6_add_repc_replicas(proxy_multiprocess_client
    ../subclass.rep
)

#### Keys ignored in scope 1:.:.:client.pro:<TRUE>:
# TEMPLATE = "app"
