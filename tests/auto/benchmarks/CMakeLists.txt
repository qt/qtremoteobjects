
#####################################################################
## tst_benchmarkstest Test:
#####################################################################

qt_internal_add_test(tst_benchmarkstest
    SOURCES
        tst_benchmarkstest.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::RemoteObjects
)
qt6_add_repc_sources(tst_benchmarkstest
    ../repfiles/localdatacenter.rep
    ../repfiles/tcpdatacenter.rep
)
qt6_add_repc_replicas(tst_benchmarkstest
        ../repfiles/localdatacenter.rep
        ../repfiles/tcpdatacenter.rep
)

#### Keys ignored in scope 1:.:.:benchmarks.pro:<TRUE>:
# TEMPLATE = "app"
