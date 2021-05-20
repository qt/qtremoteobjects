#####################################################################
## tst_parser Test:
#####################################################################

set(REPPARSER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/repparser")

qt_internal_add_test(tst_parser
    SOURCES
        tst_parser.cpp
    INCLUDE_DIRECTORIES
        ${REPPARSER_DIR}
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

# QLALR Grammars:
qt_process_qlalr(
    tst_parser
    ${REPPARSER_DIR}/parser.g
    ""
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_parser CONDITION MSVC
    COMPILE_OPTIONS
        /wd4129
)
