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

#include "repparser.h"

#include <QDebug>
#include <QTextStream>

ASTProperty::ASTProperty()
    : modifier(ReadWrite)
{
}

ASTProperty::ASTProperty(const QString &type, const QString &name, const QString &defaultValue, Modifier modifier)
    : type(type), name(name), defaultValue(defaultValue), modifier(modifier)
{
}

ASTFunction::ASTFunction(const QString &name, const QString &returnType)
    : returnType(returnType), name(name)
{
}

QString ASTFunction::paramsAsString(ParamsAsStringFormat format) const
{
    QString str;
    foreach (const ASTDeclaration &param, params) {
        if (param.variableType & ASTDeclaration::Constant)
            str += QLatin1String("const ");
        str += param.type;
        if (param.variableType & ASTDeclaration::Reference)
            str += QLatin1String(" &");
        if (format == WithVariableNames) {
            str += QString::fromLatin1(" %1").arg(param.name);
        }
        str += QStringLiteral(", ");
    }
    str.chop(2);
    return str;
}

QStringList ASTFunction::paramNames() const
{
    QStringList names;
    foreach (const ASTDeclaration &param, params) {
        names << param.name;
    }
    return names;
}

ASTClass::ASTClass(const QString &name)
    : name(name)
{
}

bool ASTClass::isValid() const
{
    return !name.isEmpty();
}

QRegExp re_class(QStringLiteral("class\\s*(\\S+)\\s*"));
QRegExp re_pod(QStringLiteral("POD\\s*(\\S+)\\s*\\(\\s*(.*)\\s*\\);?\\s*"));
QRegExp re_prop(QStringLiteral("\\s*PROP\\s*\\(([^\\)]+)\\);?.*"));
QRegExp re_useEnum(QStringLiteral("USE_ENUM\\s*\\(\\s*(.*)\\s*\\);?\\s*"));
QRegExp re_signal(QStringLiteral("\\s*SIGNAL\\s*\\(\\s*(.*)\\s*\\(\\s*(.*)\\s*\\)\\s*\\);?\\s*"));
QRegExp re_slot(QStringLiteral("\\s*SLOT\\s*\\(\\s*(.*)\\s*\\(\\s*(.*)\\s*\\)\\s*\\);?\\s*"));
QRegExp re_start(QStringLiteral("^\\{\\s*"));
QRegExp re_end(QStringLiteral("^\\};?\\s*"));
QRegExp re_comment(QStringLiteral("^\\s*//(.*)"));

RepParser::RepParser(QIODevice &outputDevice)
    : m_outputDevice(outputDevice)
{
}

bool RepParser::parse()
{
    // clean up from previous run
    m_ast = AST();

    ASTClass astClass;

    QTextStream stream(&m_outputDevice);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();

        if (re_class.exactMatch(line)) {
            // new Class declaration
            astClass = ASTClass(re_class.capturedTexts().at(1));
        } else if (re_pod.exactMatch(line)) {
            POD pod;
            pod.name = re_pod.capturedTexts().at(1);

            const QString argString = re_pod.capturedTexts().at(2).trimmed();
            if (argString.isEmpty())
                continue;
            RepParser::TypeParser parseType;
            parseType.parseArguments(argString);
            parseType.appendPods(pod);
            m_ast.pods.append(pod);
        } else if (re_useEnum.exactMatch(line)) {
            m_ast.enumUses.append(re_useEnum.capturedTexts().at(1));
        } else if (re_prop.exactMatch(line)) {
            const QStringList params = re_prop.capturedTexts();
            if (!parseProperty(astClass, params.at(1)))
                return false;
        } else if (re_signal.exactMatch(line)) {
            const QStringList captures = re_signal.capturedTexts();

            ASTFunction signal;
            signal.name = captures.at(1).trimmed();

            const QString argString = captures.at(2).trimmed();
            RepParser::TypeParser parseType;
            parseType.parseArguments(argString);
            parseType.appendParams(signal);
            astClass.signalsList << signal;
        } else if (re_slot.exactMatch(line)) {
            const QStringList captures = re_slot.capturedTexts();

            QString returnTypeAndName = captures.at(1).trimmed();

            // compat code with old SLOT declaration: "SLOT(func(...))"
            const bool hasWhitespace = returnTypeAndName.indexOf(QStringLiteral(" ")) != -1;
            if (!hasWhitespace) {
                qWarning() << "[repc] - Adding 'void' for unspecified return type on" << qPrintable(captures.at(0).trimmed());
                returnTypeAndName.prepend(QStringLiteral("void "));
            }

            const int startOfFunctionName = returnTypeAndName.lastIndexOf(QStringLiteral(" ")) + 1;

            ASTFunction slot;
            slot.returnType = returnTypeAndName.mid(0, startOfFunctionName-1);
            slot.name = returnTypeAndName.mid(startOfFunctionName);

            const QString argString = captures.at(2).trimmed();
            RepParser::TypeParser parseType;
            parseType.parseArguments(argString);
            parseType.appendParams(slot);
            astClass.slotsList << slot;
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
        const QChar inputChar(input.at(i));
        if (inputChar == QLatin1Char('<')) {
            propertyType += inputChar;
            inTemplate = true;
            ++templateDepth;
        } else if (inputChar == QLatin1Char('>')) {
            propertyType += inputChar;
            --templateDepth;
            if (templateDepth == 0)
                inTemplate = false;
        } else if (inputChar == QLatin1Char(' ')) {
            if (!inTemplate) {
                nameIndex = i;
                break;
            } else {
                propertyType += inputChar;
            }
        } else {
            propertyType += inputChar;
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

    astClass.properties << ASTProperty(propertyType, propertyName, propertyDefaultValue, propertyModifier);
    return true;
}

AST RepParser::ast() const
{
    return m_ast;
}


void RepParser::TypeParser::parseArguments(const QString &arguments)
{
    int templateDepth = 0;
    bool inTemplate = false;
    bool inVariable = false;
    QString propertyType;
    QString variableName;
    ASTDeclaration::VariableTypes variableType = ASTDeclaration::None;
    int variableNameIndex = 0;
    for (int i = 0; i < arguments.size(); ++i) {
        const QChar inputChar(arguments.at(i));
        if (inputChar == QLatin1Char('<')) {
            propertyType += inputChar;
            inTemplate = true;
            ++templateDepth;
        } else if (inputChar == QLatin1Char('>')) {
            propertyType += inputChar;
            --templateDepth;
            if (templateDepth == 0)
                inTemplate = false;
        } else if (inputChar == QLatin1Char(' ')) {
            if (inTemplate)
                propertyType += inputChar;
            else if (!propertyType.isEmpty()) {
                if (propertyType == QLatin1String("const")) {
                    propertyType.clear();
                    variableType |= ASTDeclaration::Constant;
                } else {
                    inVariable = true;
                }
            }
        } else if (inputChar == QLatin1Char('&')) {
            variableType |= ASTDeclaration::Reference;
        } else if (inputChar == QLatin1Char(',')) {
            if (!inTemplate) {
                RepParser::TypeParser::generateFunctionParameter(variableName, propertyType, variableNameIndex, variableType);
                propertyType.clear();
                variableName.clear();
                inVariable = false;
            } else {
                propertyType += inputChar;
            }
        } else {
            if (inVariable)
                variableName += inputChar;
            else
                propertyType += inputChar;
        }
    }
    if (!propertyType.isEmpty()) {
        RepParser::TypeParser::generateFunctionParameter(variableName, propertyType, variableNameIndex, variableType);
    }
}


void RepParser::TypeParser::generateFunctionParameter(QString variableName, const QString &propertyType, int &variableNameIndex, ASTDeclaration::VariableTypes variableType)
{
    if (!variableName.isEmpty())
        variableName = variableName.trimmed();
    else
        variableName = QString::fromLatin1("__repc_variable_%1").arg(++variableNameIndex);
    arguments.append(ASTDeclaration(propertyType, variableName, variableType));
}


void RepParser::TypeParser::appendParams(ASTFunction &slot)
{
    Q_FOREACH (const ASTDeclaration &arg, arguments) {
        slot.params << arg;
    }
}

void RepParser::TypeParser::appendPods(POD &pods)
{
    Q_FOREACH (const ASTDeclaration &arg, arguments) {
        PODAttribute attr;
        attr.type = arg.type;
        attr.name = arg.name;
        pods.attributes.append(qMove(attr));
    }
}
