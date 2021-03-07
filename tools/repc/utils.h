/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef UTILS_H
#define UTILS_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QTextStream>

QT_BEGIN_NAMESPACE

class QJsonValue;
class QJsonArray;
struct AST;
struct ASTClass;
struct ASTEnum;
struct ASTProperty;
struct POD;

QByteArray generateClass(const QJsonValue &cls, bool alwaysGenerateClass = false);
AST classList2AST(const QJsonArray &classes);

// The Qt/C++ generator is not easy to refactor at this point, so GeneratorBase
// has the shared components used by all generators
class GeneratorBase
{
public:

    enum class Mode
    {
        Replica,
        Source,
        SimpleSource,
        Merged
    };

    GeneratorBase();
    virtual ~GeneratorBase();

protected:
    QByteArray classSignature(const ASTClass &ac);
    QByteArray enumSignature(const ASTEnum &e);
    QByteArray podSignature(const POD &pod);
    QHash<QString, QByteArray> m_globalEnumsPODs;
};

// GeneratorImplBase has shared components used by new generators
class GeneratorImplBase : public GeneratorBase
{
public:
    GeneratorImplBase(QTextStream &_stream);

    virtual bool generateClass(const ASTClass &astClass, Mode mode) = 0;
    virtual void generateEnum(const ASTEnum &astEnum) = 0;
    virtual void generatePod(const POD &pod) = 0;
    virtual void generatePrefix(const AST &) {}
    virtual void generateSuffix(const AST &) {}
    bool hasNotify(const ASTProperty &property, Mode mode);
    bool hasPush(const ASTProperty &property, Mode mode);
    bool hasSetter(const ASTProperty &property, Mode mode);

protected:
    QTextStream &stream;
};

QT_END_NAMESPACE

#endif // UTILS_H
