# Generated from repparser.pro.

#####################################################################
## tst_parser Test:
#####################################################################

qt_add_test(tst_parser
    SOURCES
        tst_parser.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_SOURCE_DIR}/src/repparser
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

# QLALR Grammars:
qt_process_qlalr(
    tst_parser
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/repparser/parser.g # special case
    ""
)

## Scopes:
#####################################################################

qt_extend_target(tst_parser CONDITION MSVC
    COMPILE_OPTIONS
        /wd4129
)
