#
# Copyright (C) 2019 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of the QtRepc module of the Qt Toolkit.
#
# $QT_BEGIN_LICENSE:LGPL$
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU Lesser General Public License Usage
# Alternatively, this file may be used under the terms of the GNU Lesser
# General Public License version 3 as published by the Free Software
# Foundation and appearing in the file LICENSE.LGPL3 included in the
# packaging of this file. Please review the following information to
# ensure the GNU Lesser General Public License version 3 requirements
# will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 2.0 or (at your option) the GNU General
# Public license version 3 or any later version approved by the KDE Free
# Qt Foundation. The licenses are as published by the Free Software
# Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-2.0.html and
# https://www.gnu.org/licenses/gpl-3.0.html.
#
# $QT_END_LICENSE$

function(qt6_add_repc_files type target)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs FILES)

    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(outfiles)
    set(repc_incpath) ########### TODO

    foreach(it ${ARGS_FILES})
        get_filename_component(outfilename ${it} NAME_WE)
        get_filename_component(extension ${it} EXT)
        if ("${extension}" STREQUAL ".h" OR "${extension}" STREQUAL ".hpp")
            qt_manual_moc(qtro_moc_files OUTPUT_MOC_JSON_FILES json_list ${it})
            set(infile ${json_list})
            set_source_files_properties(${qtro_moc_files} PROPERTIES HEADER_FILE_ONLY ON)
            list(APPEND outfiles ${qtro_moc_files})
        else()
            get_filename_component(infile ${it} ABSOLUTE)
        endif()
        set(outfile ${CMAKE_CURRENT_BINARY_DIR}/rep_${outfilename}_${type}.h)
        add_custom_command(
            OUTPUT ${outfile}
            ${QT_TOOL_PATH_SETUP_COMMAND}
            COMMAND
                ${QT_CMAKE_EXPORT_NAMESPACE}::repc
                -o ${type} ${repc_incpath} ${infile} ${outfile}
            MAIN_DEPENDENCY ${infile}
            DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::repc
            VERBATIM
        )
        list(APPEND outfiles ${outfile})
    endforeach()
    target_sources(${target} PRIVATE ${outfiles})
endfunction()

# Add .rep source files to a target to generate source header files
function(qt6_add_repc_sources target)
    list(POP_FRONT ARGV)
    qt6_add_repc_files(source ${target} FILES ${ARGV})
endfunction()

# Add .rep source files to a target to generate replica header files
function(qt6_add_repc_replicas target)
    list(POP_FRONT ARGV)
    qt6_add_repc_files(replica ${target} FILES ${ARGV})
endfunction()

# Add .rep source files to a target to generate combined (source and replica) header files
function(qt6_add_repc_merged target)
    list(POP_FRONT ARGV)
    qt6_add_repc_files(merged ${target} FILES ${ARGV})
endfunction()

# Create .rep interface file from QObject header
function(qt6_rep_from_header target)
    list(POP_FRONT ARGV)
    foreach(it ${ARGV})
        get_filename_component(outfilename ${it} NAME_WE)
        qt_manual_moc(qtro_moc_files OUTPUT_MOC_JSON_FILES json_list ${it})
        set(infile ${json_list})
        set_source_files_properties(${qtro_moc_files} PROPERTIES HEADER_FILE_ONLY ON)
        list(APPEND outfiles ${qtro_moc_files})
        set(outfile ${CMAKE_CURRENT_BINARY_DIR}/${outfilename}.rep)
        add_custom_command(OUTPUT ${outfile}
                           ${QT_TOOL_PATH_SETUP_COMMAND}
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::repc
                           -o rep ${infile} ${outfile}
                           MAIN_DEPENDENCY ${infile}
                           VERBATIM)
        list(APPEND outfiles ${outfile})
    endforeach()
    target_sources(${target} PRIVATE ${outfiles})
endfunction()
