/****************************************************************************
**
** Copyright (C) 2021 Ford Motor Company
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

#include "pythoncodegenerator.h"
#include "repparser.h"
#include "utils.h"

#include <QScopedPointer>
#include <QStringList>
#include <QTextStream>

typedef QLatin1String l1;

static QString cap(QString name)
{
    if (!name.isEmpty())
        name[0] = name[0].toUpper();
    return name;
}

class PythonGenerator : public GeneratorImplBase
{
public:
    PythonGenerator(QTextStream &_stream) : GeneratorImplBase(_stream) {}
    bool generateClass(const ASTClass &astClass, Mode mode) override;
    void generateEnum(const ASTEnum &) override {}
    void generatePod(const POD &) override {}
    QString typeLookup(const QString &type) const;
    QString paramString(const QStringList &params) const;
    QString tupleString(const QStringList &packParams) const;
};

class PysideGenerator : public GeneratorImplBase
{
public:
    PysideGenerator(QTextStream &_stream) : GeneratorImplBase(_stream) {}
    bool generateClass(const ASTClass &astClass, Mode mode) override;
    void generateEnum(const ASTEnum &) override {}
    void generatePod(const POD &) override {}
    void generatePrefix(const AST &) override;
    QString typeLookup(const QString &type) const;
    QString paramString(const QStringList &params) const;
    QString tupleString(const QStringList &packParams) const;
};

bool PythonGenerator::generateClass(const ASTClass &astClass, Mode mode)
{
    if (mode != Mode::Replica)  // For now support Replica only
        return true;

    bool validClass = true;
    QStringList signalNames;
    QStringList propertyPackTypes, propertyDefaults;
    for (const auto &prop : astClass.properties) {
        signalNames << l1("%1Changed").arg(prop.name);
        auto packType = typeLookup(prop.type);
        if (packType.isEmpty()) {
            qWarning() << "Invalid property" << prop.name << "of type" << prop.type
                       << "as the type isn't convertible to/from python.";
            validClass = false;
            continue;
        }

        propertyPackTypes << packType;
        propertyDefaults << (prop.defaultValue.isNull() ? l1("None") : prop.defaultValue);
    }

    QList<QStringList> signalPackParameters;
    for (const auto &sig : astClass.signalsList) {
        signalNames << sig.name;
        QStringList packParams;
        for (const auto &param : sig.params) {
            auto paramType = typeLookup(param.type);
            if (paramType.isEmpty()) {
                qWarning() << "Invalid signal parameter of" << sig.name << "of type" << param.type
                           << "as the type isn't convertible to/from python.";
                validClass = false;
                continue;
            }
            packParams << paramType;
        }
        signalPackParameters << packParams;
    }

    QStringList slotPackReturnValues;
    QList<QStringList> slotPackParameters;
    for (const auto &slot : astClass.slotsList) {
        QStringList packParams;
        if (slot.returnType == QLatin1String("void")) {
            slotPackReturnValues << QString();
        } else {
            auto returnType = typeLookup(slot.returnType);
            if (returnType.isEmpty()) {
                qWarning() << "Invalid slot return type of" << slot.name << "of type"
                           << slot.returnType << "as the type isn't convertible to/from python.";
                validClass = false;
                continue;
            }
            slotPackReturnValues << returnType;
        }
        for (const auto &param : slot.params) {
            auto paramType = typeLookup(param.type);
            if (paramType.isEmpty()) {
                qWarning() << "Invalid slot parameter of" << slot.name << "of type" << param.type
                           << "as the type isn't convertible to/from python.";
                validClass = false;
                continue;
            }
            packParams << paramType;
        }
        slotPackParameters << packParams;
    }

    if (!validClass)
        return false;

    stream << "@dataclass(eq=False)\n";
    stream << "class " << astClass.name << "(Replica):\n";
    if (signalNames.count()) {
        stream << "    Signals = ['" << signalNames.join(l1("', '")) << "']\n";
        for (const auto &name : signalNames)
            stream << "    " << name << ": Signal = field(init=False, repr=False, "
                   << "default_factory=Signal)\n";
    }

    int index = -1;
    int methodIndex = 0;
    for (const auto &prop : astClass.properties) {
        index++;
        stream << "\n";
        stream << "    @property\n";
        stream << "    def " << prop.name << "(self):\n";
        stream << "        return self._priv.properties[" << index << "]\n";
        switch (prop.modifier) {
            case ASTProperty::Constant:
            case ASTProperty::ReadOnly:
            case ASTProperty::SourceOnlySetter:
            case ASTProperty::ReadPush:
                stream << "\n";
                stream << "    def push" << cap(prop.name) << "(self, " << prop.name << "):\n";
                stream << "        print('push" << cap(prop.name) << "', " << prop.name << ")\n";
                stream << "        self.callSlot(" << methodIndex++ << ", " << prop.name << ")\n";
                break;
            case ASTProperty::ReadWrite:
                stream << "\n";
                stream << "    @" << prop.name << ".setter\n";
                stream << "    def " << prop.name << "(self, " << prop.name << "):\n";
                stream << "        print('" << prop.name << " setter', " << prop.name << ")\n";
                stream << "        self.callSetter(" << index << ", " << prop.name << ")\n";
                break;
        }
    }

    for (const auto &slot : astClass.slotsList) {
        static bool firstLine = true;
        if (firstLine) {
            stream << "\n";
            firstLine = false;
        }
        QStringList paramNames;
        for (const auto &param : slot.params)
            paramNames << param.name;
        auto parameters = paramString(paramNames);
        stream << "    def " << slot.name << "(self" << parameters << "):\n";
        stream << "        print('Calling " << slot.name << " slot')\n";
        stream << "        self.callSlot(" << methodIndex++ << parameters << ")\n";
    }

    stream << "\n";
    stream << "    @classmethod\n";
    stream << "    def defaults(cls):\n";
    stream << "        return [" << propertyDefaults.join(l1(", ")) << "]\n";

    stream << "\n";
    stream << "    @classmethod\n";
    stream << "    def propTypes(cls):\n";
    stream << "        return [\n";
    for (int i = 0; i < astClass.properties.count(); i++) {
        const auto prop = astClass.properties.at(i);
        stream << "            ('" << propertyPackTypes.at(i) << "',), # " <<  prop.name
               << " is of type " << prop.type << " -> " << propertyPackTypes.at(i) << "\n";
    }
    stream << "        ]\n";

    stream << "\n";
    stream << "    @classmethod\n";
    stream << "    def signalTypes(cls):\n";
    stream << "        return [\n";
    for (int i = 0; i < astClass.properties.count(); i++)
        stream << "            ('" << propertyPackTypes.at(i) << "',),\n";
    for (int i = 0; i < astClass.signalsList.count(); i++)
        stream << "            " << tupleString(signalPackParameters.at(i)) << ",\n";
    stream << "        ]\n";

    stream << "\n";
    stream << "    @classmethod\n";
    stream << "    def slotTypes(cls):\n";
    stream << "        return [\n";
    for (int i = 0; i < astClass.properties.count(); i++) {
        auto prop = astClass.properties.at(i);
        if (prop.modifier == ASTProperty::ReadPush)
            stream << "            ('" << propertyPackTypes.at(i) << "',),\n";
    }
    for (int i = 0; i < astClass.slotsList.count(); i++)
        stream << "            " << tupleString(slotPackParameters.at(i)) << ",\n";
    stream << "        ]\n";

    stream << "\n";
    stream << "    @classmethod\n";
    stream << "    def signature(cls):\n";
    stream << "        return b'" << classSignature(astClass) << "'\n";

    stream << "\n";
    stream << "    @classmethod\n";
    stream << "    def name(cls):\n";
    stream << "        return '" << astClass.name << "'\n";

    return true;
}

QString PythonGenerator::typeLookup(const QString &type) const
{
    const static QMap<QString, QString> primitiveTypes{
        {l1("char"), l1("c")},
        {l1("bool"), l1("?")},
        {l1("short"), l1("h")},
        {l1("unsigned short"), l1("H")},
        {l1("int"), l1("i")},
        {l1("unsigned int"), l1("I")},
        {l1("long"), l1("l")},
        {l1("unsigned long"), l1("L")},
        {l1("long long"), l1("q")},
        {l1("unsigned long long"), l1("Q")},
        {l1("float"), l1("f")},
        {l1("double"), l1("d")},
    };
    if (primitiveTypes.contains(type))
        return primitiveTypes[type];

    return {};
}

QString PythonGenerator::paramString(const QStringList &params) const
{
    // Return a string to add to a function call of parameters
    // This will either follow "self" or an index, so it should
    // be an empty string or start with a ","
    if (params.count() == 0)
        return QString();

    return l1(", %1").arg(params.join(l1(", ")));
}

QString PythonGenerator::tupleString(const QStringList &packParams) const
{
    // Return a string for a proper python tuple for the classmethods
    if (packParams.count() == 0)
        return l1("()");

    if (packParams.count() > 1)
        return l1("('%1')").arg(packParams.join(l1("', '")));

    return l1("('%1',)").arg(packParams.at(0));
}

bool PysideGenerator::generateClass(const ASTClass &astClass, Mode mode)
{
    if (mode == Mode::SimpleSource) {
        stream << "class " << astClass.name << "SimpleSource(" << astClass.name << "Source):\n\n";
        stream << "    def __init__(self, parent=None):\n";
        stream << "        super().__init__(parent)\n";
        for (const auto &prop : astClass.properties) {
            if (!prop.defaultValue.isNull())
                stream << "        self._" << prop.name << " = " << prop.type << "("
                       << prop.defaultValue << ")\n";
        }

        for (const auto &prop : astClass.properties) {
            stream << "\n";
            stream << "    def get" << cap(prop.name) << "(self):\n";
                stream << "        return self._" << prop.name << "\n";
            if (hasPush(prop, mode))
            {
                stream << "\n";
                stream << "    def push" << cap(prop.name) << "(self, " << prop.name << "):\n";
                stream << "        self.set" <<  cap(prop.name) << "(" << prop.name << ")\n";
            }
            if (hasSetter(prop, mode))
            {
                stream << "\n";
                stream << "    def set" << cap(prop.name) << "(self, " << prop.name << "):\n";
                stream << "        if " << prop.name << " != self._" << prop.name << ":\n";
                stream << "            self._" << prop.name << " = " << prop.name << "\n";
                stream << "            self." << prop.name << "Changed.emit("
                       << prop.name << ")\n";
            }
        }
        stream << "\n";
        return true;
    }

    stream << "@ClassInfo({'RemoteObject Type':'" << astClass.name
           << "', 'RemoteObject Signature':'" << classSignature(astClass) << "'})\n";

    if (mode == Mode::Source)
        stream << "class " << astClass.name
               << "Source(QObject, Generic[T], metaclass=QABCMeta):\n";
    else
        stream << "class " << astClass.name << "Replica(QRemoteObjectReplica):\n";
    for (const auto &prop : astClass.properties) {
        if (hasNotify(prop, mode))
            stream << "    " << l1("%1Changed").arg(prop.name) << " = Signal("
                   << prop.type << ")\n";
    }
    for (const auto &sig : astClass.signalsList) {
        if (sig.params.count() == 0) {
            stream << "    " << sig.name << " = Signal()\n";
        } else {
            stream << "    " << sig.name << " = Signal(" << sig.params.at(0).type;
            for (int index = 1; index < sig.params.count(); index++)
                stream << ", " << sig.params.at(index).type;
            stream << ")\n";
        }
    }
    int indexCount = 0;
    if (mode == Mode::Replica) {
        for (const auto &prop : astClass.properties) {
            if (hasPush(prop, mode)) {
                indexCount++;
                stream << "    push" << cap(prop.name) << "_index = -1\n";
            } else if (hasSetter(prop, mode)) {
                indexCount++;
                stream << "    set" << cap(prop.name) << "_index = -1\n";
            }
        }
        for (const auto &slot : astClass.slotsList) {
            stream << "    " << slot.name << "_index = -1\n";
            indexCount++;
        }
    }
    stream << "\n";
    if (mode == Mode::Source) {
        stream << "    def __init__(self, parent=None):\n";
        stream << "        super().__init__(parent)\n";
    } else {
        stream << "    def __init__(self, node=None, name=None):\n";
        stream << "        super().__init__()\n";
        stream << "        self.initialize()\n";
        if (indexCount > 0) {
            bool first = true;
            for (const auto &prop : astClass.properties) {
                if (hasPush(prop, mode)) {
                    if (first) {
                        stream << "        if " << astClass.name << "Replica.push"
                               << cap(prop.name) << "_index == -1:\n";
                        first = false;
                    }
                    stream << "            " << astClass.name << "Replica.push" << cap(prop.name)
                           << "_index = self.metaObject().indexOfSlot('push" << cap(prop.name)
                           << "(" << prop.type <<  ")')\n";
                } else if (hasSetter(prop, mode)) {
                    if (first) {
                        stream << "        if " << astClass.name << "Replica.set"
                               << cap(prop.name) << "_index == -1:\n";
                        first = false;
                    }
                    stream << "            " << astClass.name << "Replica.set" << cap(prop.name)
                           << "_index = self.metaObject().indexOfProperty('"
                           << prop.name << "')\n";
                }
            }
            for (const auto &slot : astClass.slotsList) {
                if (first) {
                    stream << "        if " << astClass.name << "Replica."  << slot.name
                           << "_index == -1:\n";
                    first = false;
                }
                stream << "            " << astClass.name << "Replica."  << slot.name
                       << "_index = self.metaObject().indexOfSlot('" << slot.name << "("
                       << slot.paramsAsString(ASTFunction::Normalized) << ")')\n";
            }

        }
        stream << "        if node:\n";
        stream << "            self.initializeNode(node, name)\n\n";
        stream << "    def initialize(self):\n";
        stream << "        self.setProperties([";
        bool first = true;
        for (const auto &prop : astClass.properties) {
            if (!first)
                stream << ", ";
            stream << prop.type << "(" << prop.defaultValue << ")";
            first = false;
        }
        stream << "])\n";
    }

    int index = -1;
    for (const auto &prop : astClass.properties) {
        index++;
        stream << "\n";
        if (mode == Mode::Source) {
            stream << "    def _get" << cap(prop.name) << "(self):\n";
            stream << "        return self.get" << cap(prop.name) << "()\n\n";
            stream << "    @abstractmethod\n";
        }
        stream << "    def get" << cap(prop.name) << "(self):\n";
        if (mode == Mode::Source)
            stream << "        pass\n";
        else
            stream << "        return self.propAsVariant(" << index << ")\n";
        if (hasSetter(prop, mode)) {
            stream << "\n";
            if (mode == Mode::Source) {
                stream << "    def _set" << cap(prop.name) << "(self, " << prop.name << "):\n";
                stream << "        return self.set" << cap(prop.name) << "(" << prop.name
                       << ")\n\n";
                stream << "    @abstractmethod\n";
            }
            stream << "    def set" << cap(prop.name) << "(self, " << prop.name << "):\n";
            if (mode == Mode::Source)
                stream << "        pass\n";
            else
                stream << "        self.send(QMetaObject.WriteProperty, " << astClass.name
                       << "Replica.set" << cap(prop.name) << "_index, [" << prop.name << "])\n";
        }
        if (hasPush(prop, mode)) {
            stream << "\n";
            if (mode == Mode::Source)
                stream << "    @abstractmethod\n";
            else
                stream << "    @Slot(" << prop.type << ")\n";
            stream << "    def push" << cap(prop.name) << "(self, " << prop.name << "):\n";
            if (mode == Mode::Source) {
                stream << "        pass\n\n";
                stream << "    @Slot(" << prop.type << ", name='push" << cap(prop.name) << "')\n";
                stream << "    def _push" << cap(prop.name) << "(self, " << prop.name << "):\n";
                stream << "        return self.push" << cap(prop.name) << "(" << prop.name
                       << ")\n";
            } else {
                stream << "        self.send(QMetaObject.InvokeMetaMethod, " << astClass.name
                       << "Replica.push" << cap(prop.name) << "_index, [" << prop.name << "])\n";
            }
        }
        stream << "\n";
        if (mode == Mode::Source)
            stream << "    " << prop.name << " = Property(" << prop.type << ", _get"
                   << cap(prop.name);
        else
            stream << "    " << prop.name << " = Property(" << prop.type << ", get"
                   << cap(prop.name);
        if (hasSetter(prop, mode)) {
            if (mode == Mode::Source)
                stream << ", _set" << cap(prop.name);
            else
                stream << ", set" << cap(prop.name);
        }
        if (hasNotify(prop, mode))
            stream << ", notify = " << prop.name << "Changed";
        stream << ")\n";
    }

    stream << "\n";
    for (const auto &slot : astClass.slotsList) {
        static bool firstLine = true;
        if (firstLine) {
            stream << "\n";
            firstLine = false;
        }
        QStringList names, types;
        for (const auto &param : slot.params) {
            names << param.name;
            types << param.type;
        }
        if (mode == Mode::Source)
            stream << "    @abstractmethod\n";
        else
            stream << "    @Slot(" << types.join(l1(", ")) << ")\n";
        if (names.count() == 0)
            stream << "    def " << slot.name << "(self):\n";
        else
            stream << "    def " << slot.name << "(self, " << names.join(l1(", ")) << "):\n";
        if (mode == Mode::Source) {
            stream << "        pass\n\n";
            stream << "    @Slot(" << types.join(l1(", ")) << (names.count() == 0 ? "" : ", ")
                << "name='" << slot.name << "')\n";
            stream << "    def _" << slot.name << "(self" << (names.count() == 0 ? "" : ", ")
                   << names.join(l1(", ")) << "):\n";
            stream << "        return self." << slot.name << "(" << names.join(l1(", "))
                   << ")\n";
        } else {
            stream << "        self.send(QMetaObject.InvokeMetaMethod, " << astClass.name
                   << "Replica." << slot.name << "_index, [" << names.join(l1(", ")) << "])\n";
        }
    }
    stream << "\n";
    return true;
}

void PysideGenerator::generatePrefix(const AST &)
{
    stream << "from abc import abstractmethod, ABC, ABCMeta\n";
    stream << "from typing import TypeVar, Generic, Iterator\n";
    stream << "from PySide6.QtCore import (ClassInfo, Property, QMetaObject,\n";
    stream << "                            QObject, Signal, Slot)\n";
    stream << "from PySide6.QtRemoteObjects import QRemoteObjectReplica\n\n";

    stream << "QObjectType = type(QObject)\n";
    stream << "T = TypeVar('T')\n\n";

    stream << "class QABCMeta(QObjectType, ABCMeta):\n";
    stream << "    pass\n\n";
}

QT_BEGIN_NAMESPACE

PythonCodeGenerator::PythonCodeGenerator(QIODevice *outputDevice)
    : m_outputDevice(outputDevice)
{
    Q_ASSERT(m_outputDevice);
}

void PythonCodeGenerator::generate(const AST &ast, OutputStyle style)
{
    QTextStream stream(m_outputDevice);

    QScopedPointer<GeneratorImplBase> generator;
    if (style == OutputStyle::DataStream)
        generator.reset(new PythonGenerator(stream));
    else
        generator.reset(new PysideGenerator(stream));

    generator->generatePrefix(ast);

    for (const ASTEnum &en : ast.enums)
        generator->generateEnum(en);

    for (const POD &pod : ast.pods)
        generator->generatePod(pod);

    for (const ASTClass &astClass : ast.classes) {
        generator->generateClass(astClass, GeneratorBase::Mode::Source);
        generator->generateClass(astClass, GeneratorBase::Mode::SimpleSource);
        generator->generateClass(astClass, GeneratorBase::Mode::Replica);
    }

    generator->generateSuffix(ast);
}

QT_END_NAMESPACE
