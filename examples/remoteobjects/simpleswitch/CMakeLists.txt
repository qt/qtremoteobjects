# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT ANDROID)
    qt_internal_add_example(directconnectclient)
    qt_internal_add_example(directconnectserver)
endif()
