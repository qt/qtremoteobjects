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

#ifndef QREMOTEOBJECTSOURCE_P_H
#define QREMOTEOBJECTSOURCE_P_H

#include <QObject>
#include <QMetaObject>
#include <QMetaProperty>
#include <QVector>
#include "qremoteobjectsource.h"

QT_BEGIN_NAMESPACE

class QRemoteObjectSourceIo;
class ServerIoDevice;

class QRemoteObjectSource : public QObject
{
public:
    explicit QRemoteObjectSource(QObject *object, const SourceApiMap *,
                                 QObject *adapter, QRemoteObjectSourceIo *sourceIo);

    ~QRemoteObjectSource();

    int qt_metacall(QMetaObject::Call call, int methodId, void **a);
    QList<ServerIoDevice*> listeners;
    QObject *m_object, *m_adapter;
    const SourceApiMap * const m_api;
    QRemoteObjectSourceIo *m_sourceIo;
    bool hasAdapter() const { return m_adapter; }

    QVariantList marshalArgs(int index, void **a);
    void handleMetaCall(int index, QMetaObject::Call call, void **a);
    void addListener(ServerIoDevice *io, bool dynamic = false);
    int removeListener(ServerIoDevice *io, bool shouldSendRemove = false);
    bool invoke(QMetaObject::Call c, bool forAdapter, int index, const QVariantList& args, QVariant* returnValue = Q_NULLPTR);
    static const int qobjectPropertyOffset;
    static const int qobjectMethodOffset;
};

class DynamicApiMap : public SourceApiMap
{
public:
    DynamicApiMap(const QMetaObject *meta, const QString &name);
    ~DynamicApiMap() {}
    QString name() const Q_DECL_OVERRIDE { return _name; }
    int propertyCount() const Q_DECL_OVERRIDE { return _properties.size(); }
    int signalCount() const Q_DECL_OVERRIDE { return _signals.size(); }
    int methodCount() const Q_DECL_OVERRIDE { return _methods.size(); }
    int sourcePropertyIndex(int index) const Q_DECL_OVERRIDE
    {
        if (index < 0 || index >= propertyCount())
            return -1;
        return _properties.at(index);
    }
    int sourceSignalIndex(int index) const Q_DECL_OVERRIDE
    {
        if (index < 0 || index >= signalCount())
            return -1;
        return _signals.at(index);
    }
    int sourceMethodIndex(int index) const Q_DECL_OVERRIDE
    {
        if (index < 0 || index >= methodCount())
            return -1;
        return _methods.at(index);
    }
    int signalParameterCount(int index) const Q_DECL_OVERRIDE { return parameterCount(_signals.at(index)); }
    int signalParameterType(int sigIndex, int paramIndex) const Q_DECL_OVERRIDE { return parameterType(_signals.at(sigIndex), paramIndex); }
    const QByteArray signalSignature(int index) const Q_DECL_OVERRIDE { return signature(_signals.at(index)); }
    int methodParameterCount(int index) const Q_DECL_OVERRIDE { return parameterCount(_methods.at(index)); }
    int methodParameterType(int methodIndex, int paramIndex) const Q_DECL_OVERRIDE { return parameterType(_methods.at(methodIndex), paramIndex); }
    const QByteArray methodSignature(int index) const Q_DECL_OVERRIDE { return signature(_methods.at(index)); }
    QMetaMethod::MethodType methodType(int index) const Q_DECL_OVERRIDE;
    const QByteArray typeName(int index) const Q_DECL_OVERRIDE;
    int propertyIndexFromSignal(int index) const Q_DECL_OVERRIDE
    {
        if (index >= 0 && index < _propertyAssociatedWithSignal.size())
            return _propertyAssociatedWithSignal.at(index);
        return -1;
    }
    bool isDynamic() const Q_DECL_OVERRIDE { return true; }
private:
    int parameterCount(int objectIndex) const;
    int parameterType(int objectIndex, int paramIndex) const;
    const QByteArray signature(int objectIndex) const;
    inline void checkCache(int objectIndex) const
    {
        if (objectIndex != _cachedMetamethodIndex) {
            _cachedMetamethodIndex = objectIndex;
            _cachedMetamethod = _meta->method(objectIndex);
        }
    }

    QString _name;
    QVector<int> _properties;
    QVector<int> _signals;
    QVector<int> _methods;
    QVector<int> _propertyAssociatedWithSignal;
    const QMetaObject *_meta;
    mutable QMetaMethod _cachedMetamethod;
    mutable int _cachedMetamethodIndex;
};

QT_END_NAMESPACE

#endif
