/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef REPPARSER_H
#define REPPARSER_H

#include <QStringList>
#include <QVector>

// A property of a Class declaration
class ASTProperty
{
public:
    enum Modifier
    {
        Constant,
        ReadOnly,
        ReadWrite
    };

    ASTProperty();
    ASTProperty(const QString &type, const QString &name, const QString &defaultValue, Modifier modifier);

    QString type() const;
    QString name() const;
    QString defaultValue() const;
    Modifier modifier() const;

private:
    QString m_type;
    QString m_name;
    QString m_defaultValue;
    Modifier m_modifier;
};
Q_DECLARE_TYPEINFO(ASTProperty, Q_MOVABLE_TYPE);

// A Class declaration
class ASTClass
{
public:
    explicit ASTClass(const QString& name = QString());
    bool isValid() const;
    QString name() const;
    QVector<ASTProperty> properties() const;
    QStringList slotsList() const;
    QStringList signalsList() const;
private:
    friend class RepParser;
    friend class RepCodeGenerator;

    QString m_name;
    QVector<ASTProperty> m_properties;
    QStringList m_signals;
    QStringList m_slots;
};
Q_DECLARE_TYPEINFO(ASTClass, Q_MOVABLE_TYPE);

// The attribute of a POD
struct PODAttribute
{
    QString type;
    QString name;
};
Q_DECLARE_TYPEINFO(PODAttribute, Q_MOVABLE_TYPE);

// A POD declaration
struct POD
{
    QString name;
    QVector<PODAttribute> attributes;
};
Q_DECLARE_TYPEINFO(POD, Q_MOVABLE_TYPE);

// The AST representation of a .rep file
class AST
{
public:
    QVector<ASTClass> classes;
    QVector<POD> pods;
    QVector<QString> enumUses;
    QStringList includes;
};
Q_DECLARE_TYPEINFO(AST, Q_MOVABLE_TYPE);

class RepParser
{
public:
    explicit RepParser(const QString &fileName);

    bool parse();

    AST ast() const;

private:
    bool parseProperty(ASTClass &astClass, const QString &propertyDeclaration);

    QString m_fileName;
    AST m_ast;
};

#endif
