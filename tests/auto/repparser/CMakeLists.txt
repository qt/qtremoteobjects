# Generated from repparser.pro.

#####################################################################
## tst_parser Test:
#####################################################################

add_qt_test(tst_parser
    SOURCES
        tst_parser.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/repparser # special case
    LIBRARIES
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

extend_target(tst_parser CONDITION MSVC
    COMPILE_OPTIONS
        /wd4129
)
