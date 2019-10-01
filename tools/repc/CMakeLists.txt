# Generated from repc.pro.

#####################################################################
## repc Tool:
#####################################################################

add_qt_tool(repc
    TOOLS_TARGET RemoteObjects # special case
    SOURCES
        cppcodegenerator.cpp cppcodegenerator.h
        main.cpp
        moc_copy/moc.cpp moc_copy/moc.h
        moc_copy/outputrevision.h
        moc_copy/parser.cpp moc_copy/parser.h
        moc_copy/preprocessor.cpp moc_copy/preprocessor.h
        moc_copy/symbols.h
        moc_copy/token.cpp moc_copy/token.h
        moc_copy/utils.h
        repcodegenerator.cpp repcodegenerator.h
        utils.cpp utils.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_CAST_TO_ASCII
        QT_NO_URL_CAST_FROM_STRING
        RO_INSTALL_HEADERS=\"$$[QT_INSTALL_HEADERS]/QtRemoteObjects\"
    INCLUDE_DIRECTORIES
        ${CMAKE_SOURCE_DIR}/src/repparser
        moc_copy
    LIBRARIES
        Qt::CorePrivate
)

# QLALR Grammars:
qt_process_qlalr(
    repc
    ${CMAKE_SOURCE_DIR}/src/repparser/parser.g
    ""
)

#### Keys ignored in scope 1:.:.:repc.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Qt Remote Objects Compiler"
# _LOADED = "qt_tool"
# _OPTION = "host_build"

## Scopes:
#####################################################################

extend_target(repc CONDITION MSVC
    COMPILE_OPTIONS
        /wd4129
)
