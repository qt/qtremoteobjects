# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_add_library(repclib
    export_header.h
)

target_link_libraries(repclib PUBLIC
    Qt::RemoteObjects
)
target_compile_definitions(repclib PRIVATE QT_BUILD_REPCLIB_LIB)
set_target_properties(repclib PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    OBJC_VISIBILITY_PRESET hidden
    OBJCXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
)


qt6_add_repc_merged(repclib
    classwithattributetest.rep
)

qt_internal_add_test(tst_repcinlib
    SOURCES
        tst_repcinlib.cpp
    LIBRARIES
        repclib
)

