# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if (NOT ANDROID)
    qt_internal_add_example(directconnectclient)
    qt_internal_add_example(directconnectserver)
endif()
