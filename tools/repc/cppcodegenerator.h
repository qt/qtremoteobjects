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
******************************************************************************/

#ifndef CPPCODEGENERATOR_H
#define CPPCODEGENERATOR_H

QT_BEGIN_NAMESPACE
class QJsonArray;
class QIODevice;

class CppCodeGenerator
{
public:
    CppCodeGenerator(QIODevice *outputDevice);

    void generate(const QJsonArray &classList, bool alwaysGenerateClass = false);

private:
    QIODevice *m_outputDevice;
};

QT_END_NAMESPACE

#endif // CPPCODEGENERATOR_H
