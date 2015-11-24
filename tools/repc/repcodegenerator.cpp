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
#include <QMetaType>
#include <QTextStream>

QT_USE_NAMESPACE

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

/*
  Returns \c true if the type is a built-in type.
*/
static bool isBuiltinType(const QString &type)
 {
    int id = QMetaType::type(type.toLatin1().constData());
    if (id == QMetaType::UnknownType)
        return false;
    return (id < QMetaType::User);
}

RepCodeGenerator::RepCodeGenerator(QIODevice *outputDevice)
    : m_outputDevice(outputDevice)
{
    Q_ASSERT(m_outputDevice);
}

void RepCodeGenerator::generate(const AST &ast, Mode mode, QString fileName)
{
    QTextStream stream(m_outputDevice);
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
    foreach (const ASTEnum &en, ast.enums)
        generateENUM(stream, en);
    foreach (const POD &pod, ast.pods)
        generatePOD(stream, pod);

    QSet<QString> metaTypes;
    foreach (const POD &pod, ast.pods)
        metaTypes << pod.name;
    foreach (const ASTClass &astClass, ast.classes) {
        foreach (const ASTProperty &property, astClass.properties)
            metaTypes << property.type;
        foreach (const ASTFunction &function, astClass.signalsList + astClass.slotsList) {
            metaTypes << function.returnType;
            foreach (const ASTDeclaration &decl, function.params) {
                metaTypes << decl.type;
            }
        }
    }

    const QString metaTypeRegistrationCode = generateMetaTypeRegistration(metaTypes)
                                           + generateMetaTypeRegistrationForEnums(ast.enumUses);

    foreach (const ASTClass &astClass, ast.classes) {
        if (mode == MERGED) {
            generateClass(REPLICA, out, astClass, metaTypeRegistrationCode);
            generateClass(SOURCE, out, astClass, metaTypeRegistrationCode);
            generateClass(SIMPLE_SOURCE, out, astClass, metaTypeRegistrationCode);
            generateSourceAPI(out, astClass);
        } else {
            generateClass(mode, out, astClass, metaTypeRegistrationCode);
            if (mode == SOURCE) {
                generateClass(SIMPLE_SOURCE, out, astClass, metaTypeRegistrationCode);
                generateSourceAPI(out, astClass);
            }
        }
    }

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
           "#include <QtCore/qobject.h>\n"
           "#include <QtCore/qdatastream.h>\n"
           "#include <QtCore/qvariant.h>\n"
           "#include <QtCore/qmetatype.h>\n"
           "\n"
           "#include <QtRemoteObjects/qremoteobjectnode.h>\n"
           ;
    if (mode == MERGED) {
        out << "#include <QtRemoteObjects/qremoteobjectpendingcall.h>\n";
        out << "#include <QtRemoteObjects/qremoteobjectreplica.h>\n";
        out << "#include <QtRemoteObjects/qremoteobjectsource.h>\n";
    } else if (mode == REPLICA) {
        out << "#include <QtRemoteObjects/qremoteobjectpendingcall.h>\n";
        out << "#include <QtRemoteObjects/qremoteobjectreplica.h>\n";
    } else
        out << "#include <QtRemoteObjects/qremoteobjectsource.h>\n";
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
    return formatTemplateStringArgTypeNameCapitalizedName(1, 3, QStringLiteral("    Q_PROPERTY(%1 %2 READ %2 WRITE set%3)\n"), pod);
}

QString RepCodeGenerator::formatConstructors(const POD &pod)
{
    QString initializerString = QStringLiteral(": ");
    QString defaultInitializerString = initializerString;
    QString argString;
    foreach (const PODAttribute &a, pod.attributes) {
        initializerString += QString::fromLatin1("_%1(%1), ").arg(a.name);
        defaultInitializerString += QString::fromLatin1("_%1(), ").arg(a.name);
        argString += QString::fromLatin1("%1 %2, ").arg(a.type, a.name);
    }
    argString.chop(2);
    initializerString.chop(2);
    defaultInitializerString.chop(2);

    return QString::fromLatin1("    %1() %2 {}\n"
                   "    explicit %1(%3) %4 {}\n")
            .arg(pod.name, defaultInitializerString, argString, initializerString);
}

QString RepCodeGenerator::formatPropertyGettersAndSetters(const POD &pod)
{
    // MSVC doesn't like adjacent string literal concatenation within QStringLiteral, so keep it in one line:
    QString templateString
            = QStringLiteral("    %1 %2() const { return _%2; }\n    void set%3(%1 %2) { if (%2 != _%2) { _%2 = %2; } }\n");
    return formatTemplateStringArgTypeNameCapitalizedName(2, 8, qMove(templateString), pod);
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
    QStringList equalityCheck;
    foreach (const PODAttribute &attr, pod.attributes) {
        equalityCheck << QStringLiteral("left.%1() == right.%1()").arg(attr.name);
    }
    out << "class " << pod.name << "\n"
           "{\n"
           "    Q_GADGET\n"
        <<      formatQPropertyDeclarations(pod)
        << "public:\n"
        <<      formatConstructors(pod)
        <<      formatPropertyGettersAndSetters(pod)
        << "private:\n"
        <<      formatDataMembers(pod)
        << "};\n"
        << "\n"
        << "inline bool operator==(const " << pod.name << " &left, const " << pod.name << " &right) Q_DECL_NOTHROW {\n"
        << "    return " << equalityCheck.join(QStringLiteral(" && ")) << ";\n"
        << "}\n"
        << "inline bool operator!=(const " << pod.name << " &left, const " << pod.name << " &right) Q_DECL_NOTHROW {\n"
        << "    return !(left == right);\n"
        << "}\n"
        << "\n"
        << formatMarshallingOperators(pod)
        << "\n"
           "\n"
           ;
}

QString getEnumType(const ASTEnum &en)
{
    if (en.isSigned) {
        if (en.max < 0x7F)
            return QStringLiteral("qint8");
        if (en.max < 0x7FFF)
            return QStringLiteral("qint16");
        return QStringLiteral("qint32");
    } else {
        if (en.max < 0xFF)
            return QStringLiteral("quint8");
        if (en.max < 0xFFFF)
            return QStringLiteral("quint16");
        return QStringLiteral("quint32");
    }
}

void RepCodeGenerator::generateENUM(QTextStream &out, const ASTEnum &en)
{
    const QString type = getEnumType(en);
    out << "class " << en.name << "Enum\n"
           "{\n"
           "    Q_GADGET\n"
           "#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))\n"
           "    Q_ENUMS(" << en.name << ")\n"
           "#endif\n"
           "public:\n"
           "    enum " << en.name << "{";
        foreach (const ASTEnumParam &p, en.params)
            out << p.name << " = " << p.value << ",";

    out << "};\n"
           "#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))\n"
           "    Q_ENUM(" << en.name << ")\n"
           "#endif\n\n"
           "    static inline " << en.name << " fromInt(" << type << " i, bool *ok = 0)\n"
           "    {\n"
           "        if (ok)\n"
           "            *ok = true;\n"
           "        switch (i) {\n";
        foreach (const ASTEnumParam &p, en.params)
            out << "        case " << p.value << ": return " << p.name << ";\n";
    out << "        default:\n"
           "            if (ok)\n"
           "                *ok = false;\n"
           "            return " << en.params.at(0).name << ";\n"
           "        }\n"
           "    }\n"
           "};\n"
           "\n"
           ;

    out <<  "#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))\n"
            "  Q_DECLARE_METATYPE(" << en.name <<"Enum::" << en.name << ")\n"
            "#endif\n\n";

    out <<  "inline QDataStream &operator<<(QDataStream &ds, const " << en.name << "Enum::" << en.name << " &obj) {\n"
            "    " << type << " val = obj;\n"
            "    ds << val;\n"
            "    return ds;\n"
            "}\n\n"

            "inline QDataStream &operator>>(QDataStream &ds, " << en.name << "Enum::" << en.name << " &obj) {\n"
            "    bool ok;\n"
            "    " << type << " val;\n"
            "    ds >> val;\n"
            "    obj = " << en.name << "Enum::fromInt(val, &ok);\n"
            "    if (!ok)\n        qWarning() << \"QtRO received an invalid enum value for type" << en.name << ", value =\" << val;\n"
            "    return ds;\n"
            "}\n\n";
}

QString RepCodeGenerator::generateMetaTypeRegistration(const QSet<QString> &metaTypes)
{
    QString out;
    const QString qRegisterMetaType = QStringLiteral("        qRegisterMetaType<");
    const QString qRegisterMetaTypeStreamOperators = QStringLiteral("        qRegisterMetaTypeStreamOperators<");
    const QString lineEnding = QStringLiteral(">();\n");
    foreach (const QString &metaType, metaTypes) {
        if (isBuiltinType(metaType))
            continue;

        out += qRegisterMetaType;
        out += metaType;
        out += lineEnding;

        out += qRegisterMetaTypeStreamOperators;
        out += metaType;
        out += lineEnding;
    }
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
    const QString className = (astClass.name + (mode == REPLICA ? QStringLiteral("Replica") : mode == SOURCE ? QStringLiteral("Source") : QStringLiteral("SimpleSource")));
    if (mode == REPLICA)
        out << QString::fromLatin1("class %1 : public QRemoteObjectReplica").arg(className);
    else
        out << QString::fromLatin1("class %1 : public QObject").arg(className);

    out << QStringLiteral("{");
    out << QStringLiteral("    Q_OBJECT");
    out << QStringLiteral("    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, \"%1\")").arg(astClass.name);
    out << QStringLiteral("    friend class QRemoteObjectNode;");
    if (mode == REPLICA)
        out <<QStringLiteral("private:");
    else
        out <<QStringLiteral("public:");

    if (mode == REPLICA) {
        out << QStringLiteral("    %1() : QRemoteObjectReplica() {}").arg(className);
        out << QStringLiteral("    void initialize()");
    } else {
        out << QStringLiteral("    explicit %1(QObject *parent = Q_NULLPTR) : QObject(parent)").arg(className);

        if (mode == SIMPLE_SOURCE) {
            foreach (const ASTProperty &property, astClass.properties) {
                if (!property.defaultValue.isEmpty()) {
                    out += QStringLiteral("        , _%1(%2)").arg(property.name).arg(property.defaultValue);
                } else {
                    out += QStringLiteral("        , _%1()").arg(property.name);
                }
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
            out << QStringLiteral("        return propAsVariant(%1).value<%2 >();").arg(i).arg(property.type);
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
    } else if (mode == SOURCE) {
        foreach (const ASTProperty &property, astClass.properties)
            out << QString::fromLatin1("    virtual %1 %2() const = 0;").arg(property.type, property.name);
        foreach (const ASTProperty &property, astClass.properties) {
            if (property.modifier == ASTProperty::ReadWrite)
                out << QStringLiteral("    virtual void set%3(%1 %2) = 0;").arg(property.type, property.name, cap(property.name));
        }
    } else {
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
                out << QStringLiteral("            Q_EMIT %1Changed(_%1);").arg(property.name);
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
                out << QStringLiteral("    void %1Changed(%2);").arg(property.name).arg(property.type);
        }

        foreach (const ASTFunction &signal, astClass.signalsList)
            out << QStringLiteral("    void %1(%2);").arg(signal.name).arg(signal.paramsAsString());
    }
    if (!astClass.slotsList.isEmpty()) {
        out << QStringLiteral("");
        out << QStringLiteral("public Q_SLOTS:");
        foreach (const ASTFunction &slot, astClass.slotsList) {
            if (mode != REPLICA) {
                out << QStringLiteral("    virtual %1 %2(%3) = 0;").arg(slot.returnType).arg(slot.name).arg(slot.paramsAsString());
            } else {
                // TODO: Discuss whether it is a good idea to special-case for void here,
                const bool isVoid = slot.returnType == QStringLiteral("void");

                if (isVoid)
                    out << QStringLiteral("    void %1(%2)").arg(slot.name).arg(slot.paramsAsString());
                else
                    out << QStringLiteral("    QRemoteObjectPendingReply<%1> %2(%3)").arg(slot.returnType).arg(slot.name).arg(slot.paramsAsString());
                out << QStringLiteral("    {");
                out << QStringLiteral("        static int __repc_index = %1::staticMetaObject.indexOfSlot(\"%2(%3)\");")
                    .arg(className).arg(slot.name).arg(slot.paramsAsString(ASTFunction::Normalized));
                out << QStringLiteral("        QVariantList __repc_args;");
                if (!slot.paramNames().isEmpty()) {
                    QStringList variantNames;
                    foreach (const QString &name, slot.paramNames())
                        variantNames << QStringLiteral("QVariant::fromValue(%1)").arg(name);

                    out << QStringLiteral("        __repc_args << %1;").arg(variantNames.join(QLatin1String(" << ")));
                }
                if (isVoid)
                    out << QStringLiteral("        send(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args);");
                else
                    out << QStringLiteral("        return QRemoteObjectPendingReply<%1>(sendWithReply(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args));").arg(slot.returnType);
                out << QStringLiteral("    }");
            }
        }
    }

    if (mode == SIMPLE_SOURCE)
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

void RepCodeGenerator::generateSourceAPI(QStringList &out, const ASTClass &astClass)
{
    const QString className = astClass.name + QStringLiteral("SourceAPI");
    out << QStringLiteral("template <class ObjectType>");
    out << QString::fromLatin1("struct %1 : public SourceApiMap").arg(className);
    out << QStringLiteral("{");
    out << QString::fromLatin1("    %1()").arg(className);
    out << QStringLiteral("    {");
    const int propCount = astClass.properties.count();
    out << QString::fromLatin1("        _properties[0] = %1;").arg(propCount);
    QStringList changeSignals;
    QList<int> propertyChangeIndex;
    for (int i = 0; i < propCount; ++i) {
        const ASTProperty &prop = astClass.properties.at(i);
        out << QString::fromLatin1("        _properties[%1] = qtro_prop_index<ObjectType>(&ObjectType::%2, "
                              "static_cast<%3 (QObject::*)()>(0),\"%2\");")
                             .arg(i+1).arg(prop.name).arg(prop.type);
        if (prop.modifier == prop.ReadWrite) //Make sure we have a setter function
            out << QStringLiteral("        qtro_method_test<ObjectType>(&ObjectType::set%1, static_cast<void (QObject::*)(%2)>(0));")
                                 .arg(cap(prop.name)).arg(prop.type);
        if (prop.modifier != prop.Constant) { //Make sure we have an onChange signal
            out << QStringLiteral("        qtro_method_test<ObjectType>(&ObjectType::%1Changed, static_cast<void (QObject::*)()>(0));")
                                 .arg(prop.name);
            changeSignals << QString::fromLatin1("%1Changed").arg(prop.name);
            propertyChangeIndex << i + 1; //_properties[0] is the count, so index is one higher
        }
    }
    const int signalCount = astClass.signalsList.count();
    const int changedCount = changeSignals.size();
    out << QString::fromLatin1("        _signals[0] = %1;").arg(signalCount+changeSignals.size());
    for (int i = 0; i < changedCount; ++i)
        out << QString::fromLatin1("        _signals[%1] = qtro_signal_index<ObjectType>(&ObjectType::%2, "
                              "static_cast<void (QObject::*)()>(0),signalArgCount+%4,signalArgTypes[%4]);")
                             .arg(i+1).arg(changeSignals.at(i)).arg(i);
    for (int i = 0; i < signalCount; ++i) {
        const ASTFunction &sig = astClass.signalsList.at(i);
        out << QString::fromLatin1("        _signals[%1] = qtro_signal_index<ObjectType>(&ObjectType::%2, "
                              "static_cast<void (QObject::*)(%3)>(0),signalArgCount+%4,signalArgTypes[%4]);")
                             .arg(changedCount+i+1).arg(sig.name).arg(sig.paramsAsString(ASTFunction::Normalized)).arg(i);
    }
    const int slotCount = astClass.slotsList.count();
    out << QString::fromLatin1("        _methods[0] = %1;").arg(slotCount);
    for (int i = 0; i < slotCount; ++i) {
        const ASTFunction &slot = astClass.slotsList.at(i);
        out << QString::fromLatin1("        _methods[%1] = qtro_method_index<ObjectType>(&ObjectType::%2, "
                              "static_cast<void (QObject::*)(%3)>(0),\"%2(%3)\",methodArgCount+%4,methodArgTypes[%4]);")
                             .arg(i+1).arg(slot.name).arg(slot.paramsAsString(ASTFunction::Normalized)).arg(i);
    }

    out << QStringLiteral("    }");
    out << QStringLiteral("");
    out << QString::fromLatin1("    QString name() const Q_DECL_OVERRIDE { return QStringLiteral(\"%1\"); }").arg(astClass.name);
    out << QStringLiteral("    int propertyCount() const Q_DECL_OVERRIDE { return _properties[0]; }");
    out << QStringLiteral("    int signalCount() const Q_DECL_OVERRIDE { return _signals[0]; }");
    out << QStringLiteral("    int methodCount() const Q_DECL_OVERRIDE { return _methods[0]; }");
    out << QStringLiteral("    int sourcePropertyIndex(int index) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    out << QStringLiteral("        if (index < 0 || index >= _properties[0])");
    out << QStringLiteral("            return -1;");
    out << QStringLiteral("        return _properties[index+1];");
    out << QStringLiteral("    }");
    out << QStringLiteral("    int sourceSignalIndex(int index) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    out << QStringLiteral("        if (index < 0 || index >= _signals[0])");
    out << QStringLiteral("            return -1;");
    out << QStringLiteral("        return _signals[index+1];");
    out << QStringLiteral("    }");
    out << QStringLiteral("    int sourceMethodIndex(int index) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    out << QStringLiteral("        if (index < 0 || index >= _methods[0])");
    out << QStringLiteral("            return -1;");
    out << QStringLiteral("        return _methods[index+1];");
    out << QStringLiteral("    }");
    if (signalCount+changedCount > 0) {
        out << QStringLiteral("    int signalParameterCount(int index) const Q_DECL_OVERRIDE");
        out << QStringLiteral("    {");
        out << QStringLiteral("        if (index < 0 || index >= _signals[0])");
        out << QStringLiteral("            return -1;");
        out << QStringLiteral("        return signalArgCount[index];");
        out << QStringLiteral("    }");
        out << QStringLiteral("    int signalParameterType(int sigIndex, int paramIndex) const Q_DECL_OVERRIDE");
        out << QStringLiteral("    {");
        out << QStringLiteral("        if (sigIndex < 0 || sigIndex >= _signals[0] || paramIndex < 0 || paramIndex >= signalArgCount[sigIndex])");
        out << QStringLiteral("            return -1;");
        out << QStringLiteral("        return signalArgTypes[sigIndex][paramIndex];");
        out << QStringLiteral("    }");
    } else {
        out << QStringLiteral("    int signalParameterCount(int index) const Q_DECL_OVERRIDE { Q_UNUSED(index); return -1; }");
        out << QStringLiteral("    int signalParameterType(int sigIndex, int paramIndex) const Q_DECL_OVERRIDE");
        out << QStringLiteral("    { Q_UNUSED(sigIndex); Q_UNUSED(paramIndex); return -1; }");
    }
    if (slotCount) {
        out << QStringLiteral("    int methodParameterCount(int index) const Q_DECL_OVERRIDE");
        out << QStringLiteral("    {");
        out << QStringLiteral("        if (index < 0 || index >= _methods[0])");
        out << QStringLiteral("            return -1;");
        out << QStringLiteral("        return methodArgCount[index];");
        out << QStringLiteral("    }");
        out << QStringLiteral("    int methodParameterType(int methodIndex, int paramIndex) const Q_DECL_OVERRIDE");
        out << QStringLiteral("    {");
        out << QStringLiteral("        if (methodIndex < 0 || methodIndex >= _methods[0] || paramIndex < 0 || paramIndex >= methodArgCount[methodIndex])");
        out << QStringLiteral("            return -1;");
        out << QStringLiteral("        return methodArgTypes[methodIndex][paramIndex];");
        out << QStringLiteral("    }");
    } else {
        out << QStringLiteral("    int methodParameterCount(int index) const Q_DECL_OVERRIDE { Q_UNUSED(index); return -1; }");
        out << QStringLiteral("    int methodParameterType(int methodIndex, int paramIndex) const Q_DECL_OVERRIDE");
        out << QStringLiteral("    { Q_UNUSED(methodIndex); Q_UNUSED(paramIndex); return -1; }");
    }
    //propertyIndexFromSignal method
    out << QStringLiteral("    int propertyIndexFromSignal(int index) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    if (!propertyChangeIndex.isEmpty()) {
        out << QStringLiteral("        switch (index) {");
        for (int i = 0; i < propertyChangeIndex.size(); ++i)
            out << QString::fromLatin1("        case %1: return _properties[%2];").arg(i).arg(propertyChangeIndex.at(i));
        out << QStringLiteral("        }");
    } else
        out << QStringLiteral("        Q_UNUSED(index);");
    out << QStringLiteral("        return -1;");
    out << QStringLiteral("    }");
    //propertyRawIndexFromSignal method
    out << QStringLiteral("    int propertyRawIndexFromSignal(int index) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    if (!propertyChangeIndex.isEmpty()) {
        out << QStringLiteral("        switch (index) {");
        for (int i = 0; i < propertyChangeIndex.size(); ++i)
            out << QString::fromLatin1("        case %1: return %2;").arg(i).arg(propertyChangeIndex.at(i));
        out << QStringLiteral("        }");
    } else
        out << QStringLiteral("        Q_UNUSED(index);");
    out << QStringLiteral("        return -1;");
    out << QStringLiteral("    }");

    //signalSignature method
    out << QStringLiteral("    const QByteArray signalSignature(int index) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    if (signalCount+changedCount > 0) {
        out << QStringLiteral("        switch (index) {");
        for (int i = 0; i < changedCount; ++i)
            out << QString::fromLatin1("        case %1: return QByteArrayLiteral(\"%2()\");").arg(i).arg(changeSignals.at(i));
        for (int i = 0; i < signalCount; ++i)
        {
            const ASTFunction &sig = astClass.signalsList.at(i);
            out << QString::fromLatin1("        case %1: return QByteArrayLiteral(\"%2(%3)\");")
                                      .arg(i+changedCount).arg(sig.name).arg(sig.paramsAsString(ASTFunction::Normalized));
        }
        out << QStringLiteral("        }");
    } else
        out << QStringLiteral("        Q_UNUSED(index);");
    out << QStringLiteral("        return QByteArrayLiteral(\"\");");
    out << QStringLiteral("    }");

    //methodSignature method
    out << QStringLiteral("    const QByteArray methodSignature(int index) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    if (slotCount) {
        out << QStringLiteral("        switch (index) {");
        for (int i = 0; i < slotCount; ++i)
        {
            const ASTFunction &slot = astClass.slotsList.at(i);
            out << QString::fromLatin1("        case %1: return QByteArrayLiteral(\"%2(%3)\");")
                                      .arg(i).arg(slot.name).arg(slot.paramsAsString(ASTFunction::Normalized));
        }
        out << QStringLiteral("        }");
    } else
        out << QStringLiteral("        Q_UNUSED(index);");
    out << QStringLiteral("        return QByteArrayLiteral(\"\");");
    out << QStringLiteral("    }");

    //methodType method
    out << QStringLiteral("    QMetaMethod::MethodType methodType(int) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    out << QStringLiteral("        return QMetaMethod::Slot;");
    out << QStringLiteral("    }");

    //typeName method
    out << QStringLiteral("    const QByteArray typeName(int index) const Q_DECL_OVERRIDE");
    out << QStringLiteral("    {");
    if (slotCount) {
        out << QStringLiteral("        switch (index) {");
        for (int i = 0; i < slotCount; ++i)
        {
            const ASTFunction &slot = astClass.slotsList.at(i);
            out << QString::fromLatin1("        case %1: return QByteArrayLiteral(\"%2\");").arg(i).arg(slot.returnType);
        }
        out << QStringLiteral("        }");
    } else
        out << QStringLiteral("        Q_UNUSED(index);");
    out << QStringLiteral("        return QByteArrayLiteral(\"\");");
    out << QStringLiteral("    }");

    out << QStringLiteral("");
    out << QString::fromLatin1("    int _properties[%1];").arg(propCount+1);
    out << QString::fromLatin1("    int _signals[%1];").arg(signalCount+changedCount+1);
    out << QString::fromLatin1("    int _methods[%1];").arg(slotCount+1);
    if (signalCount+changedCount > 0) {
        out << QString::fromLatin1("    int signalArgCount[%1];").arg(signalCount+changedCount);
        out << QString::fromLatin1("    const int* signalArgTypes[%1];").arg(signalCount+changedCount);
    }
    out << QString::fromLatin1("    int methodArgCount[%1];").arg(slotCount);
    out << QString::fromLatin1("    const int* methodArgTypes[%1];").arg(slotCount);
    out << QStringLiteral("};");
}
