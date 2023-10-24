# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial


add_subdirectory(repparser)
if(QT_FEATURE_localserver)
    add_subdirectory(remoteobjects)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(remoteobjectsqml)
endif()
