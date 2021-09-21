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

# With a macOS framework Qt build, moc needs to be passed -F<framework-path> arguments to resolve
# framework style includes like #include <QtCore/qobject.h>
# Extract the location of the Qt frameworks by querying the imported location of QtRemoteObjects
# framework parent directory.
function(_qt_internal_get_remote_objects_framework_path out_var)
    set(value "")
    if(APPLE AND QT_FEATURE_framework)
        get_target_property(ro_path ${QT_CMAKE_EXPORT_NAMESPACE}::RemoteObjects IMPORTED_LOCATION)
        string(REGEX REPLACE "(.*)/Qt[^/]+\\.framework.*" "\\1" ro_fw_path "${ro_path}")
        if(ro_fw_path)
            set(value "${ro_fw_path}")
        endif()
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_remote_objects_framework_path_moc_options out_var)
    _qt_internal_get_remote_objects_framework_path(ro_fw_path)
    if(ro_fw_path)
        set(${out_var} "OPTIONS" "-F${ro_fw_path}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_add_repc_files type target)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs SOURCES)

    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(outfiles)
    if(QT_REPC_DEBUG_MODE)
        set(debug "-d")
    else()
        set(debug "")
    endif()
    set(repc_incpath) ########### TODO

    _qt_internal_get_remote_objects_framework_path_moc_options(extra_moc_options)
    foreach(it ${ARGS_SOURCES})
        get_filename_component(outfilename ${it} NAME_WE)
        get_filename_component(extension ${it} EXT)
        if ("${extension}" STREQUAL ".h" OR "${extension}" STREQUAL ".hpp")
            # This calls moc on an existing header file to extract metatypes json information
            # which is then passed to the tool to generate another header.
            qt6_wrap_cpp(qtro_moc_files "${it}"
                         __QT_INTERNAL_OUTPUT_MOC_JSON_FILES json_list
                         TARGET "${target}"
                         ${extra_moc_options})

            # Pass the generated metatypes .json file to the tool.
            set(infile ${json_list})
            set_source_files_properties(${qtro_moc_files} PROPERTIES HEADER_FILE_ONLY ON)
            list(APPEND outfiles ${qtro_moc_files})
        else()
            # Pass the .rep file to the tool.
            get_filename_component(infile ${it} ABSOLUTE)
        endif()
        set(outfile ${CMAKE_CURRENT_BINARY_DIR}/rep_${outfilename}_${type}.h)

        add_custom_command(
            OUTPUT ${outfile}
            ${QT_TOOL_PATH_SETUP_COMMAND}
            COMMAND
                ${QT_CMAKE_EXPORT_NAMESPACE}::repc
                ${debug} -o ${type} ${repc_incpath} ${infile} ${outfile}
            MAIN_DEPENDENCY ${infile}
            DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::repc
            VERBATIM
        )
        list(APPEND outfiles ${outfile})

        # The generated header file needs to be manually moc'ed (without using AUTOMOC) and then
        # added as source to compile into the target.
        qt6_wrap_cpp(qtro_moc_files "${outfile}" TARGET "${target}" ${extra_moc_options})
        set_source_files_properties("${outfile}" PROPERTIES
            GENERATED TRUE
            SKIP_AUTOGEN ON)
        list(APPEND outfiles ${qtro_moc_files})
    endforeach()
    target_sources(${target} PRIVATE ${outfiles})
endfunction()

# Add .rep source files to a target to generate source header files
function(qt6_add_repc_sources target)
    list(POP_FRONT ARGV)
    _qt_internal_add_repc_files(source ${target} SOURCES ${ARGV})
endfunction()

# Add .rep source files to a target to generate replica header files
function(qt6_add_repc_replicas target)
    list(POP_FRONT ARGV)
    _qt_internal_add_repc_files(replica ${target} SOURCES ${ARGV})
endfunction()

# Add .rep source files to a target to generate combined (source and replica) header files
function(qt6_add_repc_merged target)
    list(POP_FRONT ARGV)
    _qt_internal_add_repc_files(merged ${target} SOURCES ${ARGV})
endfunction()

# Create .rep interface file from QObject header
function(qt6_reps_from_headers target)
    list(POP_FRONT ARGV)
    _qt_internal_get_remote_objects_framework_path_moc_options(extra_moc_options)

    foreach(it ${ARGV})
        get_filename_component(outfilename ${it} NAME_WE)
        # This calls moc on an existing header file to extract metatypes json information
        # which is then passed to the tool to generate a .rep file.
        qt6_wrap_cpp(qtro_moc_files "${it}"
                     __QT_INTERNAL_OUTPUT_MOC_JSON_FILES json_list
                     TARGET "${target}"
                     ${extra_moc_options})
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

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_repc_sources)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_repc_sources(${ARGN})
        else()
            message(FATAL_ERROR "qt_add_repc_sources() is only available in Qt 6. "
                                "Please check the repc documentation for alternatives.")
        endif()
    endfunction()
    function(qt_add_repc_replicas)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_repc_replicas(${ARGN})
        else()
            message(FATAL_ERROR "qt_add_repc_replicas() is only available in Qt 6. "
                                "Please check the repc documentation for alternatives.")
        endif()
    endfunction()
    function(qt_add_repc_merged)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_repc_merged(${ARGN})
        else()
            message(FATAL_ERROR "qt_add_repc_merged() is only available in Qt 6. "
                                "Please check the repc documentation for alternatives.")
        endif()
    endfunction()
    function(qt_reps_from_headers)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_reps_from_headers(${ARGN})
        else()
            message(FATAL_ERROR "qt_reps_from_headers() is only available in Qt 6. "
                                "Please check the repc documentation for alternatives.")
        endif()
    endfunction()
endif()
