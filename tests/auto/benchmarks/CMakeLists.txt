# Generated from benchmarks.pro.

#####################################################################
## tst_benchmarkstest Test:
#####################################################################

qt_add_test(tst_benchmarkstest
    SOURCES
        tst_benchmarkstest.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::RemoteObjects
)
# special case begin
set(other_files
    ../repfiles/localdatacenter.rep
    ../repfiles/tcpdatacenter.rep
)
# special case end
qt6_add_repc_source(tst_benchmarkstest
    ${other_files} # special case
)
qt6_add_repc_replica(tst_benchmarkstest
    ${other_files} # special case
)

#### Keys ignored in scope 1:.:.:benchmarks.pro:<TRUE>:
# TEMPLATE = "app"
