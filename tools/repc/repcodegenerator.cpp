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

#include "repcodegenerator.h"

#include "repparser.h"

#include <QFileInfo>
#include <QTextStream>

// for normalizeTypeInternal
#include <private/qmetaobject_p.h>
#include <private/qmetaobject_moc_p.h>

QT_USE_NAMESPACE

// Code copied from moc.cpp
// We cannot depend on QMetaObject::normalizedSignature,
// since repc is linked against Qt5Bootstrap (which doesn't offer QMetaObject) when cross-compiling
// Thus, just use internal API which is exported in private headers, as moc does
static QByteArray normalizeType(const QByteArray &ba, bool fixScope = false)
{
    const char *s = ba.constData();
    int len = ba.size();
    char stackbuf[64];
    char *buf = (len >= 64 ? new char[len + 1] : stackbuf);
    char *d = buf;
    char last = 0;
    while (*s && is_space(*s))
        s++;
    while (*s) {
        while (*s && !is_space(*s))
            last = *d++ = *s++;
        while (*s && is_space(*s))
            s++;
        if (*s && ((is_ident_char(*s) && is_ident_char(last))
                   || ((*s == ':') && (last == '<')))) {
            last = *d++ = ' ';
        }
    }
    *d = '\0';
    QByteArray result = normalizeTypeInternal(buf, d, fixScope);
    if (buf != stackbuf)
        delete [] buf;
    return result;
}

template <typename C>
static int accumulatedSizeOfNames(const C &c)
{
    int result = 0;
    foreach (const typename C::value_type &e, c)
        result += e.name.size();
    return result;
}

template <typename C>
static int accumulatedSizeOfTypes(const C &c)
{
    int result = 0;
    foreach (const typename C::value_type &e, c)
        result += e.type.size();
    return result;
}

static QString cap(QString name)
{
    if (!name.isEmpty())
        name[0] = name[0].toUpper();
    return name;
}

RepCodeGenerator::RepCodeGenerator(QIODevice &outputDevice)
    : m_outputDevice(outputDevice)
{
}

void RepCodeGenerator::generate(const AST &ast, Mode mode, QString fileName)
{
    QTextStream stream(&m_outputDevice);
    if (fileName.isEmpty())
        stream << "#pragma once" << endl << endl;
    else {
        fileName = QFileInfo(fileName).fileName();
        fileName = fileName.toUpper();
        fileName.replace(QLatin1Char('.'), QLatin1Char('_'));
        stream << "#ifndef " << fileName << endl;
        stream << "#define " << fileName << endl << endl;
    }

    QStringList out;

    generateHeader(mode, stream, ast);
    foreach (const POD &pod, ast.pods)
        generatePOD(stream, pod);

    const QString metaTypeRegistrationCode = generateMetaTypeRegistrationForPODs(ast.pods)
                                           + generateMetaTypeRegistrationForEnums(ast.enumUses);

    foreach (const ASTClass &astClass, ast.classes)
        generateClass(mode, out, astClass, metaTypeRegistrationCode);

    stream << out.join(QLatin1Char('\n'));

    generateStreamOperatorsForEnums(stream, ast.enumUses);

    stream << endl;
    if (!fileName.isEmpty())
        stream << "#endif // " << fileName << endl;
}

void RepCodeGenerator::generateHeader(Mode mode, QTextStream &out, const AST &ast)
{
    out << "// This is an autogenerated file.\n"
           "// Do not edit this file, any changes made will be lost the next time it is generated.\n"
           "\n"
           "#include <QObject>\n"
           "#include <QVariantList>\n"
           "#include <QMetaProperty>\n"
           "\n"
           "#include <QRemoteObjectNode>\n"
           ;
    if (mode == REPLICA) {
        out << "#include <QRemoteObjectReplica>\n";
        out << "#include <QRemoteObjectPendingReply>\n";
    } else
        out << "#include <QRemoteObjectSource>\n";
    out << "\n";

    out << ast.includes.join(QLatin1Char('\n'));
    out << "\n";
}

static QString formatTemplateStringArgTypeNameCapitalizedName(int numberOfTypeOccurrences, int numberOfNameOccurrences,
                                                              QString templateString, const POD &pod)
{
    QString out;
    const int LengthOfPlaceholderText = 2;
    Q_ASSERT(templateString.count(QRegExp(QStringLiteral("%\\d"))) == numberOfNameOccurrences + numberOfTypeOccurrences);
    const int expectedOutSize
            = numberOfNameOccurrences * accumulatedSizeOfNames(pod.attributes)
            + numberOfTypeOccurrences * accumulatedSizeOfTypes(pod.attributes)
            + pod.attributes.size() * (templateString.size() - (numberOfNameOccurrences + numberOfTypeOccurrences) * LengthOfPlaceholderText);
    out.reserve(expectedOutSize);
    foreach (const PODAttribute &a, pod.attributes)
        out += templateString.arg(a.type, a.name, cap(a.name));
    return out;
}

QString RepCodeGenerator::formatQPropertyDeclarations(const POD &pod)
{
    return formatTemplateStringArgTypeNameCapitalizedName(1, 4, QStringLiteral("    Q_PROPERTY(%1 %2 READ %2 WRITE set%3 NOTIFY %2Changed)\n"), pod);
}

QString RepCodeGenerator::formatConstructors(const POD &pod)
{
    QString initializerString = QStringLiteral("QObject(parent)");
    QString defaultInitializerString = initializerString;
    QString argString;
    foreach (const PODAttribute &a, pod.attributes) {
        initializerString += QString::fromLatin1(", _%1(%1)").arg(a.name);
        defaultInitializerString += QString::fromLatin1(", _%1()").arg(a.name);
        argString += QString::fromLatin1("%1 %2, ").arg(a.type, a.name);
    }
    argString.chop(2);

    return QString::fromLatin1("    explicit %1(QObject *parent = Q_NULLPTR) : %2 {}\n"
                   "    explicit %1(%3, QObject *parent = Q_NULLPTR) : %4 {}\n")
            .arg(pod.name, defaultInitializerString, argString, initializerString);
}

QString RepCodeGenerator::formatCopyConstructor(const POD &pod)
{
    return QLatin1String("    ") + pod.name + QLatin1String("(const ") + pod.name + QLatin1String("& other)\n"
           "        : QObject()\n"
           "    {\n"
           "        QtRemoteObjects::copyStoredProperties(&other, this);\n"
           "    }\n"
           "\n")
           ;
}

QString RepCodeGenerator::formatCopyAssignmentOperator(const POD &pod)
{
    return QLatin1String("    ") + pod.name + QLatin1String(" &operator=(const ") + pod.name + QLatin1String(" &other)\n"
           "    {\n"
           "        if (this != &other)\n"
           "            QtRemoteObjects::copyStoredProperties(&other, this);\n"
           "        return *this;\n"
           "    }\n"
           "\n")
           ;
}

QString RepCodeGenerator::formatPropertyGettersAndSetters(const POD &pod)
{
    // MSVC doesn't like adjacent string literal concatenation within QStringLiteral, so keep it in one line:
    QString templateString
            = QStringLiteral("    %1 %2() const { return _%2; }\n    void set%3(%1 %2) { if (%2 != _%2) { _%2 = %2; Q_EMIT %2Changed(); } }\n");
    return formatTemplateStringArgTypeNameCapitalizedName(2, 9, qMove(templateString), pod);
}

QString RepCodeGenerator::formatSignals(const POD &pod)
{
    QString out;
    const QString prefix = QStringLiteral("    void ");
    const QString suffix = QStringLiteral("Changed();\n");
    const int expectedOutSize
            = accumulatedSizeOfNames(pod.attributes)
            + pod.attributes.size() * (prefix.size() + suffix.size());
    out.reserve(expectedOutSize);
    foreach (const PODAttribute &a, pod.attributes) {
        out += prefix;
        out += a.name;
        out += suffix;
    }
    Q_ASSERT(out.size() == expectedOutSize);
    return out;
}

QString RepCodeGenerator::formatDataMembers(const POD &pod)
{
    QString out;
    const QString prefix = QStringLiteral("    ");
    const QString infix  = QStringLiteral(" _");
    const QString suffix = QStringLiteral(";\n");
    const int expectedOutSize
            = accumulatedSizeOfNames(pod.attributes)
            + accumulatedSizeOfTypes(pod.attributes)
            + pod.attributes.size() * (prefix.size() + infix.size() + suffix.size());
    out.reserve(expectedOutSize);
    foreach (const PODAttribute &a, pod.attributes) {
        out += prefix;
        out += a.type;
        out += infix;
        out += a.name;
        out += suffix;
    }
    Q_ASSERT(out.size() == expectedOutSize);
    return out;
}

QString RepCodeGenerator::formatMarshallingOperators(const POD &pod)
{
    return QLatin1String("inline QDataStream &operator<<(QDataStream &ds, const ") + pod.name + QLatin1String(" &obj) {\n"
           "    QtRemoteObjects::copyStoredProperties(&obj, ds);\n"
           "    return ds;\n"
           "}\n"
           "\n"
           "inline QDataStream &operator>>(QDataStream &ds, ") + pod.name + QLatin1String(" &obj) {\n"
           "    QtRemoteObjects::copyStoredProperties(ds, &obj);\n"
           "    return ds;\n"
           "}\n")
           ;
}

void RepCodeGenerator::generatePOD(QTextStream &out, const POD &pod)
{
    out << "class " << pod.name << " : public QObject\n"
           "{\n"
           "    Q_OBJECT\n"
        <<      formatQPropertyDeclarations(pod)
        << "public:\n"
        <<      formatConstructors(pod)
        <<      formatCopyConstructor(pod)
        <<      formatCopyAssignmentOperator(pod)
        <<      formatPropertyGettersAndSetters(pod)
        << "Q_SIGNALS:\n"
        <<      formatSignals(pod)
        << "private:\n"
        <<      formatDataMembers(pod)
        << "};\n"
           "\n"
        << formatMarshallingOperators(pod)
        << "\n"
           "Q_DECLARE_METATYPE(" << pod.name << ")\n"
           "\n"
           ;
}

QString RepCodeGenerator::generateMetaTypeRegistrationForPODs(const QVector<POD> &pods)
{
    QString out;
    const QString qRegisterMetaType = QStringLiteral("        qRegisterMetaType<");
    const QString qRegisterMetaTypeStreamOperators = QStringLiteral("        qRegisterMetaTypeStreamOperators<");
    const QString lineEnding = QStringLiteral(">();\n");
    const int expectedOutSize
            = 2 * accumulatedSizeOfNames(pods)
            + pods.size() * (qRegisterMetaType.size() + qRegisterMetaTypeStreamOperators.size() + 2 * lineEnding.size());
    out.reserve(expectedOutSize);
    foreach (const POD &pod, pods) {
        out += qRegisterMetaType;
        out += pod.name;
        out += lineEnding;

        out += qRegisterMetaTypeStreamOperators;
        out += pod.name;
        out += lineEnding;
    }
    Q_ASSERT(expectedOutSize == out.size());
    return out;
}

QString RepCodeGenerator::generateMetaTypeRegistrationForEnums(const QVector<QString> &enumUses)
{
    QString out;

    foreach (const QString &enumName, enumUses) {
        out += QLatin1String("        qRegisterMetaTypeStreamOperators<") + enumName + QLatin1String(">(\"") + enumName + QLatin1String("\");\n");
    }

    return out;
}

void RepCodeGenerator::generateStreamOperatorsForEnums(QTextStream &out, const QVector<QString> &enumUses)
{
    foreach (const QString &enumName, enumUses) {
        out << "inline QDataStream &operator<<(QDataStream &out, " << enumName << " value)" << endl;
        out << "{" << endl;
        out << "    out << static_cast<qint32>(value);" << endl;
        out << "    return out;" << endl;
        out << "}" << endl;
        out << endl;
        out << "inline QDataStream &operator>>(QDataStream &in, " << enumName << " &value)" << endl;
        out << "{" << endl;
        out << "    qint32 intValue = 0;" << endl;
        out << "    in >> intValue;" << endl;
        out << "    value = static_cast<" << enumName << ">(intValue);" << endl;
        out << "    return in;" << endl;
        out << "}" << endl;
        out << endl;
    }
}

void RepCodeGenerator::generateClass(Mode mode, QStringList &out, const ASTClass &astClass, const QString &metaTypeRegistrationCode)
{
    const QString className = (astClass.name + (mode == REPLICA ? QStringLiteral("Replica") : QStringLiteral("Source")));
    if (mode == REPLICA)
        out << QString::fromLatin1("class %1 : public QRemoteObjectReplica").arg(className);
    else
        out << QString::fromLatin1("class %1 : public QRemoteObjectSource").arg(className);

    out << QStringLiteral("{");
    out << QStringLiteral("    Q_OBJECT");
    out << QStringLiteral("    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, \"%1\")").arg(astClass.name);
    out << QStringLiteral("    friend class QRemoteObjectNode;");
    if (mode == SOURCE)
        out <<QStringLiteral("public:");
    else
        out <<QStringLiteral("private:");

    if (mode == REPLICA) {
        out << QStringLiteral("    %1() : QRemoteObjectReplica() {}").arg(className);
        out << QStringLiteral("    void initialize()");
    } else {
        out << QStringLiteral("    %1(QObject *parent = Q_NULLPTR) : QRemoteObjectSource(parent)").arg(className);

        foreach (const ASTProperty &property, astClass.properties) {
            if (!property.defaultValue.isEmpty()) {
                out += QStringLiteral("        , _%1(%2)").arg(property.name).arg(property.defaultValue);
            } else {
                out += QStringLiteral("        , _%1()").arg(property.name);
            }
        }
    }

    out << QStringLiteral("    {");

    if (!metaTypeRegistrationCode.isEmpty())
        out << metaTypeRegistrationCode;

    if (mode == REPLICA) {
        out << QStringLiteral("        QVariantList properties;");
        out << QString::fromLatin1("        properties.reserve(%1);").arg(astClass.properties.size());
        foreach (const ASTProperty &property, astClass.properties) {
            out << QString(QLatin1String("        properties << QVariant::fromValue(") + property.type + QLatin1String("(") + property.defaultValue + QLatin1String("));"));
        }
        out << QStringLiteral("        setProperties(properties);");
    }

    out << QStringLiteral("    }");
    out << QStringLiteral("public:");

    out << QStringLiteral("    virtual ~%1() {}").arg(className);

    //First output properties
    foreach (const ASTProperty &property, astClass.properties) {
        if (property.modifier == ASTProperty::Constant)
            out << QStringLiteral("    Q_PROPERTY(%1 %2 READ %2 CONSTANT)").arg(property.type, property.name);
        else if (property.modifier == ASTProperty::ReadOnly)
            out << QStringLiteral("    Q_PROPERTY(%1 %2 READ %2 NOTIFY %2Changed)").arg(property.type, property.name);
        else if (property.modifier == ASTProperty::ReadWrite)
            out << QStringLiteral("    Q_PROPERTY(%1 %2 READ %2 WRITE set%3 NOTIFY %2Changed)").arg(property.type, property.name, cap(property.name));
    }
    out << QStringLiteral("");

    //Next output getter/setter
    if (mode == REPLICA) {
        int i = 0;
        foreach (const ASTProperty &property, astClass.properties) {
            out << QStringLiteral("    %1 %2() const").arg(property.type, property.name);
            out << QStringLiteral("    {");
            out << QStringLiteral("        return propAsVariant(%1).value<%2>();").arg(i).arg(property.type);
            out << QStringLiteral("    }");
            i++;
            if (property.modifier == ASTProperty::ReadWrite) {
                out << QStringLiteral("");
                out << QStringLiteral("    void set%3(%1 %2)").arg(property.type, property.name, cap(property.name));
                out << QStringLiteral("    {");
                out << QStringLiteral("        static int __repc_index = %1::staticMetaObject.indexOfProperty(\"%2\");").arg(className).arg( property.name);
                out << QStringLiteral("        QVariantList __repc_args;");
                out << QStringLiteral("        __repc_args << QVariant::fromValue(%1);").arg(property.name);
                out << QStringLiteral("        send(QMetaObject::WriteProperty, __repc_index, __repc_args);");
                out << QStringLiteral("    }");
            }
            out << QStringLiteral("");
        }
    }
    else
    {
        foreach (const ASTProperty &property, astClass.properties) {
            out << QString::fromLatin1("    virtual %1 %2() const { return _%2; }").arg(property.type, property.name);
        }
        foreach (const ASTProperty &property, astClass.properties) {
            if (property.modifier == ASTProperty::ReadWrite) {
                out << QStringLiteral("    virtual void set%3(%1 %2)").arg(property.type, property.name, cap(property.name));
                out << QStringLiteral("    {");
                out << QStringLiteral("        if (%1 != _%1)").arg(property.name);
                out << QStringLiteral("        {");
                out << QStringLiteral("            _%1 = %1;").arg(property.name);
                out << QStringLiteral("            Q_EMIT %1Changed();").arg(property.name);
                out << QStringLiteral("        }");
                out << QStringLiteral("    }");
            }
        }
    }

    //Next output property signals
    if (!astClass.properties.isEmpty() || !astClass.signalsList.isEmpty()) {
        out << QStringLiteral("");
        out << QStringLiteral("Q_SIGNALS:");
        foreach (const ASTProperty &property, astClass.properties) {
            if (property.modifier != ASTProperty::Constant)
                out << QStringLiteral("    void %1Changed();").arg(property.name);
        }

        foreach (const ASTFunction &signal, astClass.signalsList)
            out << QStringLiteral("    void %1(%2);").arg(signal.name).arg(signal.paramsAsString());
    }
    if (!astClass.slotsList.isEmpty()) {
        out << QStringLiteral("");
        out << QStringLiteral("public Q_SLOTS:");
        foreach (const ASTFunction &slot, astClass.slotsList) {
            if (mode == SOURCE) {
                out << QStringLiteral("    virtual %1 %2(%3) = 0;").arg(slot.returnType).arg(slot.name).arg(slot.paramsAsString());
            } else {
                // TODO: Discuss whether it is a good idea to special-case for void here,
                const bool isVoid = slot.returnType == QStringLiteral("void");

                if (isVoid)
                    out << QStringLiteral("    void %1(%2)").arg(slot.name).arg(slot.paramsAsString());
                else
                    out << QStringLiteral("    QRemoteObjectPendingReply<%1> %2(%3)").arg(slot.returnType).arg(slot.name).arg(slot.paramsAsString());
                out << QStringLiteral("    {");
                const QByteArray normalizedSignature = ::normalizeType(slot.paramsAsString(ASTFunction::NoVariableNames).toLatin1().constData());
                out << QStringLiteral("        static int __repc_index = %1::staticMetaObject.indexOfSlot(\"%2(%3)\");")
                    .arg(className).arg(slot.name).arg(QString::fromLatin1(normalizedSignature));
                out << QStringLiteral("        QVariantList __repc_args;");
                if (!slot.paramNames().isEmpty()) {
                    QStringList variantNames;
                    foreach (const QString &name, slot.paramNames())
                        variantNames << QStringLiteral("QVariant::fromValue(%1)").arg(name);

                    out << QStringLiteral("        __repc_args << %1;").arg(variantNames.join(QLatin1String(" << ")));
                }
                out << QStringLiteral("        qDebug() << \"%1::%2\" << __repc_index;").arg(className).arg(slot.name);
                if (isVoid)
                    out << QStringLiteral("        send(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args);");
                else
                    out << QStringLiteral("        return QRemoteObjectPendingReply<%1>(sendWithReply(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args));").arg(slot.returnType);
                out << QStringLiteral("    }");
            }
        }
    }

    if (mode == SOURCE)
    {
        //Next output data members
        if (!astClass.properties.isEmpty()) {
            out << QStringLiteral("");
            out << QStringLiteral("private:");
            foreach (const ASTProperty &property, astClass.properties) {
                out << QStringLiteral("    %1 _%2;").arg(property.type, property.name);
            }
        }
    }

    out << QStringLiteral("};");
    out << QStringLiteral("");
}
