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

#include "RepParser.h"

#include <QFile>
#include <QTextStream>

ASTProperty::ASTProperty()
    : m_modifier(ReadWrite)
{
}

ASTProperty::ASTProperty(const QString &type, const QString &name, const QString &defaultValue, Modifier modifier)
    : m_type(type), m_name(name), m_defaultValue(defaultValue), m_modifier(modifier)
{
}

QString ASTProperty::type() const
{
    return m_type;
}

QString ASTProperty::name() const
{
    return m_name;
}

QString ASTProperty::defaultValue() const
{
    return m_defaultValue;
}

ASTProperty::Modifier ASTProperty::modifier() const
{
    return m_modifier;
}


ASTClass::ASTClass(const QString &name)
    : m_name(name)
{
}

bool ASTClass::isValid() const
{
    return !m_name.isEmpty();
}

QString ASTClass::name() const
{
    return m_name;
}

QVector<ASTProperty> ASTClass::properties() const
{
    return m_properties;
}

QStringList ASTClass::slotsList() const
{
    return m_slots;
}

QStringList ASTClass::signalsList() const
{
    return m_signals;
}

//grammar
// R->WHITESPACE
// WHITESPACE: newlines, whitespaces
// R->POD
// R->ENUM
// R->CLASS DESCR
// DESCR->WHITESPACE IDENTIFIER CBRACKET
// DESCR->IDENTIFIER COBRACKET
// COBRACKET->WHITESPACE CLASSBODY
// COBRACKET->CLASSBODY
// CLASSBODY->WHITESPACE PROPERTY SEMICOLON
// CLASSBODY->PROPERTY SEMICOLON
// CLASSBODY->WHITESPACE SIGNAL SEMICOLON
// CLASSBODY->SIGNAL SEMICOLON
// CLASSBODY->WHITESPACE SLOT SEMICOLON
// CLASSBODY->SLOT SEMICOLON
// SEMICOLON->WHITESPACE CLASSBODY
// SEMICOLON->CLASSBODY
// SEMICOLON->WHITESPACE CCLOSEBRACKET
// SEMICOLON->CCLOSEBRACKET
// CCLOSEBRACKET->; WHITESPACE R
// CCLOSEBRACKET->; R
// SLOT->WHITESPACE SLOT
// SLOT->IDENTIFIER ROPENBRACKET
// ROPENBRACKET

QRegExp re_class(QStringLiteral("class\\s*(\\S+)\\s*"));
QRegExp re_pod(QStringLiteral("POD\\s*(\\S+)\\s*\\(\\s*(.*)\\s*\\);?\\s*"));
QRegExp re_prop(QStringLiteral("\\s*PROP\\s*\\(([^\\)]+)\\);?.*"));
QRegExp re_useEnum(QStringLiteral("USE_ENUM\\s*\\(\\s*(.*)\\s*\\);?\\s*"));
QRegExp re_signal(QStringLiteral("\\s*SIGNAL\\s*\\(\\s*(.*)\\s*\\);?\\s*"));
QRegExp re_slot(QStringLiteral("\\s*SLOT\\s*\\(\\s*(.*)\\s*\\);?\\s*"));
QRegExp re_start(QStringLiteral("^\\{\\s*"));
QRegExp re_end(QStringLiteral("^\\};?\\s*"));
QRegExp re_comment(QStringLiteral("^\\s*//(.*)"));

RepParser::RepParser(const QString &fileName)
    : m_fileName(fileName)
{
}

bool RepParser::parse()
{
    // clean up from previous run
    m_ast = AST();

    QFile file(m_fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    ASTClass astClass;

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();

        if (re_class.exactMatch(line)) {
            // new Class declaration
            astClass = ASTClass(re_class.capturedTexts()[1]);
        } else if (re_pod.exactMatch(line)) {
            POD pod;
            pod.name = re_pod.capturedTexts()[1];

            const QString argString = re_pod.capturedTexts()[2].trimmed();
            const QStringList argList = argString.split(QLatin1String(","));
            if (argList.length() == 0)
                continue;

            foreach (const QString &paramString, argList) {
                const QStringList tmp = paramString.trimmed().split(QRegExp(QStringLiteral("\\s+")));
                PODAttribute attr = { tmp[0], tmp[1] };
                pod.attributes.append(qMove(attr));
            }

            m_ast.pods.append(pod);
        } else if (re_useEnum.exactMatch(line)) {
            m_ast.enumUses.append(re_useEnum.capturedTexts()[1]);
        } else if (re_prop.exactMatch(line)) {
            const QStringList params = re_prop.capturedTexts();
            if (!parseProperty(astClass, params[1]))
                return false;
        } else if (re_signal.exactMatch(line)) {
            astClass.m_signals << re_signal.capturedTexts()[1];
        } else if (re_slot.exactMatch(line)) {
            astClass.m_slots << re_slot.capturedTexts()[1];
        } else if (re_end.exactMatch(line)) {
            m_ast.classes.append(astClass);
        } else if (re_start.exactMatch(line)) {
        } else {
            if (!re_comment.exactMatch(line) && !line.isEmpty()) {
               m_ast.includes.append(line);
            }
        }
    }

    return true;
}

// A helper function to parse modifier flag of property declaration
static bool parseModifierFlag(const QString &flag, ASTProperty::Modifier &modifier)
{
    if (flag == QStringLiteral("READONLY")) {
        modifier = ASTProperty::ReadOnly;
    } else if (flag == QStringLiteral("CONSTANT")) {
        modifier = ASTProperty::Constant;
    } else {
        qWarning("invalid property declaration: flag %s is unknown", qPrintable(flag));
        return false;
    }

    return true;
}

bool RepParser::parseProperty(ASTClass &astClass, const QString &propertyDeclaration)
{
    QString input = propertyDeclaration.trimmed();

    QString propertyType;
    QString propertyName;
    QString propertyDefaultValue;
    ASTProperty::Modifier propertyModifier;

    // parse type declaration which could be a nested template as well
    bool inTemplate = false;
    int templateDepth = 0;
    int nameIndex = -1;

    for (int i = 0; i < input.size(); ++i) {
        if (input[i] == QLatin1Char('<')) {
            propertyType += input[i];
            inTemplate = true;
            templateDepth++;
        } else if (input[i] == QLatin1Char('>')) {
            propertyType += input[i];
            templateDepth--;
            if (templateDepth == 0)
                inTemplate = false;
        } else if (input[i] == QLatin1Char(' ')) {
            if (!inTemplate) {
                nameIndex = i;
                break;
            } else {
                propertyType += input[i];
            }
        } else {
            propertyType += input[i];
        }
    }

    if (nameIndex == -1) {
        qWarning("invalid property declaration: %s", qPrintable(propertyDeclaration));
        return false;
    }

    // parse the name of the property
    input = input.mid(nameIndex).trimmed();

    const int equalSignIndex = input.indexOf(QLatin1Char('='));
    if (equalSignIndex != -1) { // we have a default value
        propertyName = input.left(equalSignIndex).trimmed();

        input = input.mid(equalSignIndex + 1).trimmed();
        const int whitespaceIndex = input.indexOf(QLatin1Char(' '));
        if (whitespaceIndex == -1) { // no flag given
            propertyDefaultValue = input;
            propertyModifier = ASTProperty::ReadWrite;
        } else { // flag given
            propertyDefaultValue = input.left(whitespaceIndex).trimmed();

            const QString flag = input.mid(whitespaceIndex + 1).trimmed();
            if (!parseModifierFlag(flag, propertyModifier))
                return false;
        }
    } else { // there is no default value
        const int whitespaceIndex = input.indexOf(QLatin1Char(' '));
        if (whitespaceIndex == -1) { // no flag given
            propertyName = input;
            propertyModifier = ASTProperty::ReadWrite;
        } else { // flag given
            propertyName = input.left(whitespaceIndex).trimmed();

            const QString flag = input.mid(whitespaceIndex + 1).trimmed();
            if (!parseModifierFlag(flag, propertyModifier))
                return false;
        }
    }

    astClass.m_properties << ASTProperty(propertyType, propertyName, propertyDefaultValue, propertyModifier);
    return true;
}

AST RepParser::ast() const
{
    return m_ast;
}
