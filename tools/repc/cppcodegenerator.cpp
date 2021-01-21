/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

#include "cppcodegenerator.h"
#include "moc.h"
#include "utils.h"

QT_BEGIN_NAMESPACE

CppCodeGenerator::CppCodeGenerator(QIODevice *outputDevice)
    : m_outputDevice(outputDevice)
{
    Q_ASSERT(m_outputDevice);
}

void CppCodeGenerator::generate(const QVector<ClassDef> &classList, bool alwaysGenerateClass /* = false */)
{
    for (const ClassDef &cdef : classList)
        m_outputDevice->write(generateClass(cdef, alwaysGenerateClass));

    m_outputDevice->write("\n");
}

QT_END_NAMESPACE
