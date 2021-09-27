# Generated from repc.pro.

#####################################################################
## repc Tool:
#####################################################################

set(REPPARSER_DIR "${PROJECT_SOURCE_DIR}/src/repparser")

qt_get_tool_target_name(target_name repc)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt Remote Objects Compiler"
    TOOLS_TARGET RemoteObjects # special case
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    SOURCES
        cppcodegenerator.cpp cppcodegenerator.h
        main.cpp
        repcodegenerator.cpp repcodegenerator.h
        utils.cpp utils.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_CAST_TO_ASCII
        QT_NO_URL_CAST_FROM_STRING
        RO_INSTALL_HEADERS=\"$$[QT_INSTALL_HEADERS]/QtRemoteObjects\"
    INCLUDE_DIRECTORIES
        ${REPPARSER_DIR}
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)
qt_internal_return_unless_building_tools()

# QLALR Grammars:
qt_process_qlalr(
    ${target_name}
    "${REPPARSER_DIR}/parser.g"
    ""
)

#### Keys ignored in scope 1:.:.:repc.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Qt Remote Objects Compiler"
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION MSVC
    COMPILE_OPTIONS
        /wd4129
)
