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

#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qjsonobject.h>

#include "utils.h"
#include "repparser.h"


#define _(X) QLatin1String(X)

QT_BEGIN_NAMESPACE

namespace JSON
{
    enum Types {
        Any,
        Array,
        Object,
        String,
        Bool
    };

    static QJsonValue _Sub(const QJsonValue &json, const char *key, JSON::Types type=JSON::Any) {
        if (json.isUndefined())
            qCritical() << "Invalid metadata json file. Unexpected Undefined value when looking for key:" << key;
        if (!json.isObject())
            qCritical() << "Invalid metadata json file. Input (" << json << ") is not an object when looking for key:" << key;
        QJsonValue value = json.toObject()[_(key)];
        switch (type) {
            case JSON::Any: break;
            case JSON::Array:
                if (!value.isArray())
                    qCritical() << "Invalid metadata json file. Value (" << value << ") is not an array when looking for key:" << key;
                break;
            case JSON::Object:
                if (!value.isObject())
                    qCritical() << "Invalid metadata json file. Value (" << value << ") is not an object when looking for key:" << key;
                break;
            case JSON::String:
                if (!value.isString())
                    qCritical() << "Invalid metadata json file. Value (" << value << ") is not a string when looking for key:" << key;
                break;
            case JSON::Bool:
                if (!value.isBool())
                    qCritical() << "Invalid metadata json file. Value (" << value << ") is not a bool when looking for key:" << key;
                break;
        }
        return value;
    }

    static bool _Contains(const QJsonValue &json, const char *key) {
        if (json.isUndefined())
            qCritical() << "Invalid metadata json file. Unexpected Undefined value when looking for key:" << key;
        if (!json.isObject())
            qCritical() << "Invalid metadata json file. Input (" << json << ") is not an object when looking for key:" << key;
        return json.toObject().contains(_(key));
    }

    static bool _Empty(const QJsonValue &json, const char *key) {
        if (!_Contains(json, key))
            return true;
        const auto value = _Sub(json, key);
        if (!value.isArray())
            qCritical() << "Invalid metadata json file." << key << "is not an array.";
        return value.toArray().count() == 0;
    }

    static QJsonArray _Array(const QJsonValue &json, const char *key) { return _Sub(json, key, JSON::Array).toArray(); }
    static QString _String(const QJsonValue &json, const char *key) { return _Sub(json, key, JSON::String).toString(); }
    static QByteArray _Bytes(const QJsonValue &json, const char *key) { return _Sub(json, key, JSON::String).toString().toLatin1(); }
    static bool _Bool(const QJsonValue &json, const char *key) { return _Sub(json, key, JSON::Bool).toBool(); }
    static bool _Bool(const QJsonValue &json, const char *key, bool missingValue) {
        if (!_Contains(json, key))
            return missingValue;
        bool res = _Bool(json, key);
        return res;
    }
}

using namespace JSON;

static QByteArray join(const QByteArrayList &array, const QByteArray &separator)
{
    QByteArray res;
    const auto sz = array.size();
    if (!sz)
        return res;
    for (qsizetype i = 0; i < sz - 1; i++)
        res += array.at(i) + separator;
    res += array.at(sz - 1);
    return res;
}

static QByteArrayList generateProperties(const QJsonArray &properties, bool isPod=false)
{
    QByteArrayList ret;
    for (const QJsonValue prop : properties) {
        if (!isPod && !_Contains(prop, "notify") && !_Bool(prop, "constant")) {
            qWarning() << "Skipping property" << _String(prop, "name") << "because it is non-notifiable & non-constant";
            continue; // skip non-notifiable properties
        }
        QByteArray output = _Bytes(prop, "type") + " " + _Bytes(prop, "name");
        if (_Bool(prop, "constant"))
            output += " CONSTANT";
        if (!_Contains(prop, "write") && _Contains(prop, "read."))
            output += " READONLY";
        ret << output;
    }
    return ret;
}

static QByteArray generateFunctions(const QByteArray &type, const QJsonArray &functions)
{
    QByteArray ret;
    for (const QJsonValue func : functions) {
        ret += type + "(" + _Bytes(func, "returnType") + " " + _Bytes(func, "name") + "(";
        const auto arguments = _Array(func, "arguments");
        for (const QJsonValue arg : arguments)
            ret += _Bytes(arg, "type") + " " + _Bytes(arg, "name") + ", ";
        if (arguments.count())
            ret.chop(2);
        ret += "));\n";
    }
    return ret;
}

const auto filterNotPublic = [](const QJsonValue &value) {
    return _String(value, "access") != QStringLiteral("public"); };

static QJsonArray cleanedSignalList(const QJsonValue &cls)
{
    if (_Empty(cls, "signals"))
        return QJsonArray();

    auto signalList = _Array(cls, "signals");
    if (_Empty(cls, "properties"))
        return signalList;

    const auto props = _Array(cls, "properties");
    const auto filterNotify = [&props](const QJsonValue &value) {
        const auto filter = [&value](const QJsonValue &prop) {
            return _Sub(value, "name") == _Sub(prop, "notify"); };
        return std::find_if(props.begin(), props.end(), filter) != props.end(); };
    for (auto it = signalList.begin(); it != signalList.end(); /* blank */ ) {
        if (filterNotify(*it))
            it = signalList.erase(it);
        else if (filterNotPublic(*it))
            it = signalList.erase(it);
        else
            it++;
    }
    return signalList;
}

static QJsonArray cleanedSlotList(const QJsonValue &cls)
{
    if (_Empty(cls, "slots"))
        return QJsonArray();

    auto slotList = _Array(cls, "slots");
    if (!_Empty(cls, "properties"))
        return slotList;

    const auto props = _Array(cls, "properties");
    const auto filterWrite = [&props](const QJsonValue &value) {
        const auto filter = [&value](const QJsonValue &prop) {
            const auto args = _Array(prop, "arguments");
            return _Sub(value, "name") == _Sub(prop, "write") &&
                args.count() == 1 && _Sub(args.at(0), "type") == _Sub(prop, "type"); };
        return std::find_if(props.begin(), props.end(), filter) != props.end(); };
    for (auto it = slotList.begin(); it != slotList.end(); /* blank */ ) {
        if (filterWrite(*it))
            it = slotList.erase(it);
        else if (filterNotPublic(*it))
            it = slotList.erase(it);
        else
            it++;
    }
    return slotList;
}

QByteArray generateClass(const QJsonValue &cls, bool alwaysGenerateClass)
{
    if (_Bool(cls, "gadget", false) || alwaysGenerateClass ||
            (_Empty(cls, "signals") && _Empty(cls, "slots")))
        return "POD " + _Bytes(cls, "className") + "(" + join(generateProperties(_Array(cls, "properties"), true), ", ") + ")\n";

    QByteArray ret("class " + _Bytes(cls, "className") + "\n{\n");
    if (!_Empty(cls, "properties"))
        ret += "    PROP(" + join(generateProperties(_Array(cls, "properties")), ");\n    PROP(") + ");\n";
    ret += generateFunctions("    SLOT", cleanedSlotList(cls));
    ret += generateFunctions("    SIGNAL", cleanedSignalList(cls));
    ret += "}\n";
    return ret;
}

static QList<PODAttribute> propertyList2PODAttributes(const QJsonArray &list)
{
    QList<PODAttribute> ret;
    for (const QJsonValue prop : list)
        ret.push_back(PODAttribute(_String(prop, "type"), _String(prop, "name")));
    return ret;
}

QList<ASTProperty> propertyList2AstProperties(const QJsonArray &list)
{
    QList<ASTProperty> ret;
    for (const QJsonValue property : list) {
        if (!_Contains(property, "notify") && !_Bool(property, "constant")) {
            qWarning() << "Skipping property" << _String(property, "name") << "because it is non-notifiable & non-constant";
            continue; // skip non-notifiable properties
        }
        ASTProperty prop;
        prop.name = _String(property, "name");
        prop.type = _String(property, "type");
        prop.modifier = _Bool(property, "constant")
                        ? ASTProperty::Constant
                        : !_Contains(property, "write") && _Contains(property, "read")
                          ? ASTProperty::ReadOnly
                          : ASTProperty::ReadWrite;
        ret.push_back(prop);
    }
    return ret;
}

QList<ASTFunction> functionList2AstFunctionList(const QJsonArray &list)
{
    QList<ASTFunction> ret;
    for (const QJsonValue function : list) {
        ASTFunction func;
        func.name = _String(function, "name");
        func.returnType = _String(function, "returnType");
        const auto arguments = _Array(function, "arguments");
        for (const QJsonValue arg : arguments)
            func.params.push_back(ASTDeclaration(_String(arg, "type"), _String(arg, "name")));
        ret.push_back(func);
    }
    return ret;
}

AST classList2AST(const QJsonArray &classes)
{
    AST ret;
    for (const QJsonValue cls : classes) {
        if (_Empty(cls, "signals") && _Empty(cls, "slots")) {
            POD pod;
            pod.name = _String(cls, "className");
            pod.attributes = propertyList2PODAttributes(_Array(cls, "properties"));
            ret.pods.push_back(pod);
        } else {
            ASTClass cl(_String(cls, "className"));
            cl.properties = propertyList2AstProperties(_Array(cls, "properties"));
            cl.signalsList = functionList2AstFunctionList(cleanedSignalList(cls));
            cl.slotsList = functionList2AstFunctionList(cleanedSlotList(cls));
            ret.classes.push_back(cl);
        }
    }
    return ret;
}

QT_END_NAMESPACE
