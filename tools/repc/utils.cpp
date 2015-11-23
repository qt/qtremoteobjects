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

#include "utils.h"
#include <moc.h>

#define _(X) QString::fromLatin1(X)

static QByteArray join(const QList<QByteArray> &array, const QByteArray &separator) {
    QByteArray res;
    const int sz = array.size();
    if (!sz)
        return res;
    for (int i = 0; i < sz - 1; i++)
        res += array.at(i) + separator;
    res += array.at(sz - 1);
    return res;
}

static QList<QByteArray> generateProperties(const QList<PropertyDef> &properties, bool isPod=false)
{
    QList<QByteArray> ret;
    foreach (const PropertyDef& property, properties) {
        if (!isPod && property.notifyId == -1 && !property.constant) {
            qWarning() << "Skipping property" << property.name << "because is non-notifiable & non-constant";
            continue; // skip non-notifiable properties
        }
        QByteArray prop = property.type + " " + property.name;
        if (property.constant)
            prop += " CONSTANT";
        if (property.write.isEmpty() && !property.read.isEmpty())
            prop += " READONLY";
        ret << prop;
    }
    return ret;
}

static QByteArray generateFunctions(const QByteArray &type, const QList<FunctionDef> &functionList)
{
    QByteArray ret;
    foreach (const FunctionDef &func, functionList) {
        if (func.access != FunctionDef::Public)
            continue;

        ret += type + "(" + func.name + "(";
        const int sz = func.arguments.size();
        if (sz) {
            for (int i = 0; i < sz - 1 ; i++) {
                const ArgumentDef &arg = func.arguments.at(i);
                ret += arg.normalizedType + " " + arg.name + ", ";
            }
            const ArgumentDef &arg = func.arguments.at(sz -1);
            ret += arg.normalizedType + " " + arg.name;
        }
        ret += "));\n";
    }
    return ret;
}

static bool highToLowSort(int a, int b)
{
    return a > b;
}

static QList<FunctionDef> cleanedSignalList(const ClassDef &cdef)
{
    QList<FunctionDef> ret = cdef.signalList;
    QVector<int> positions;
    foreach (const PropertyDef &prop, cdef.propertyList) {
        if (prop.notifyId != -1) {
            Q_ASSERT(prop.notify == ret.at(prop.notifyId).name);
            positions.push_back(prop.notifyId);
        }
    }
    std::sort(positions.begin(), positions.end(), highToLowSort);
    foreach (int pos, positions)
        ret.removeAt(pos);
    return ret;
}

QByteArray generateClass(const ClassDef &cdef)
{
    QList<FunctionDef> signalList = cleanedSignalList(cdef);
    if (signalList.isEmpty() && cdef.slotList.isEmpty())
        return "POD " + cdef.classname + "(" + join(generateProperties(cdef.propertyList, true), ", ") + ")\n";

    QByteArray ret("class " + cdef.classname + "\n{\n");
    ret += "    PROP(" + join(generateProperties(cdef.propertyList), ");\n    PROP(") + ");\n";
    ret += generateFunctions("    SLOT", cdef.slotList);
    ret += generateFunctions("    SIGNAL", signalList);
    ret += "}\n";
    return ret;
}

static QVector<PODAttribute> propertyList2PODAttributes(const QList<PropertyDef> &list)
{
    QVector<PODAttribute> ret;
    foreach (const PropertyDef &prop, list)
        ret.push_back(PODAttribute(_(prop.type), _(prop.name)));
    return ret;
}

QVector<ASTProperty> propertyList2AstProperties(const QList<PropertyDef> &list)
{
    QVector<ASTProperty> ret;
    foreach (const PropertyDef &property, list) {
        if (property.notifyId == -1 && !property.constant) {
            qWarning() << "Skipping property" << property.name << "because is non-notifiable & non-constant";
            continue; // skip non-notifiable properties
        }
        ASTProperty prop;
        prop.name = _(property.name);
        prop.type = _(property.type);
        prop.modifier = property.constant ? ASTProperty::Constant :
                                            property.final ? ASTProperty::ReadOnly :
                                                             ASTProperty::ReadWrite;
        ret.push_back(prop);
    }
    return ret;
}

QVector<ASTFunction> functionList2AstFunctionList(const QList<FunctionDef> &list)
{
    QVector<ASTFunction> ret;
    foreach (const FunctionDef &fdef, list) {
        if (fdef.access != FunctionDef::Public)
            continue;

        ASTFunction func;
        func.name = _(fdef.name);
        func.returnType = _(fdef.type.name);
        foreach (const ArgumentDef &arg, fdef.arguments)
            func.params.push_back(ASTDeclaration(_(arg.type.name), _(arg.name)));
        ret.push_back(func);
    }
    return ret;
}

AST classList2AST(const QList<ClassDef> &classList)
{
    AST ret;
    foreach (const ClassDef &cdef, classList) {
        QList<FunctionDef> signalList = cleanedSignalList(cdef);
        if (signalList.isEmpty() && cdef.slotList.isEmpty()) {
            POD pod;
            pod.name = _(cdef.classname);
            pod.attributes = propertyList2PODAttributes(cdef.propertyList);
            ret.pods.push_back(pod);
        } else {
            ASTClass cl(_(cdef.classname));
            cl.properties = propertyList2AstProperties(cdef.propertyList);
            cl.signalsList = functionList2AstFunctionList(cdef.signalList);
            cl.slotsList = functionList2AstFunctionList(cdef.slotList);
            ret.classes.push_back(cl);
        }
    }
    return ret;
}
