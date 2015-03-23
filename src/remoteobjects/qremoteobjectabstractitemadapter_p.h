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

#ifndef QREMOTEOBJECTS_ABSTRACT_ITEM_ADAPTER_P_H
#define QREMOTEOBJECTS_ABSTRACT_ITEM_ADAPTER_P_H

#include "qremoteobjectabstractitemmodeltypes.h"
#include "qremoteobjectsource.h"

class QAbstractItemModel;
class QItemSelectionModel;

class QAbstractItemSourceAdapter : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit QAbstractItemSourceAdapter(QAbstractItemModel *object, QItemSelectionModel *sel, const QVector<int> &roles = QVector<int>());
    Q_PROPERTY(QVector<int> availableRoles READ availableRoles WRITE setAvailableRoles NOTIFY availableRolesChanged)
    void registerTypes();
    QItemSelectionModel* selectionModel() const;

public Q_SLOTS:
    QVector<int> availableRoles() const { return m_availableRoles; }
    void setAvailableRoles(QVector<int> availableRoles)
    {
        if (availableRoles != m_availableRoles)
        {
            m_availableRoles = availableRoles;
            Q_EMIT availableRolesChanged();
        }
    }

    DataEntries replicaRowRequest(IndexList start, IndexList end, QVector<int> roles);
    QVariantList replicaHeaderRequest(QVector<Qt::Orientation> orientations, QVector<int> sections, QVector<int> roles);
    void replicaSetCurrentIndex(IndexList index, QItemSelectionModel::SelectionFlags command);

    void sourceDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles = QVector<int> ()) const;
    void sourceRowsInserted(const QModelIndex & parent, int start, int end);
    void sourceRowsRemoved(const QModelIndex & parent, int start, int end);
    void sourceRowsMoved(const QModelIndex & sourceParent, int sourceRow, int count, const QModelIndex & destinationParent, int destinationChild) const;
    void sourceCurrentChanged(const QModelIndex & current, const QModelIndex & previous);

Q_SIGNALS:
    void availableRolesChanged();
    void dataChanged(IndexList topLeft, IndexList bottomRight, QVector<int> roles) const;
    void rowsInserted(IndexList parent, int start, int end) const;
    void rowsRemoved(IndexList parent, int start, int end) const;
    void rowsMoved(IndexList sourceParent, int sourceRow, int count, IndexList destinationParent, int destinationChild) const;
    void currentChanged(IndexList current, IndexList previous);

private:
    QAbstractItemSourceAdapter();
    QAbstractItemModel *m_model;
    QItemSelectionModel *m_selectionModel;
    QVector<int> m_availableRoles;
};
namespace{
    const int s3[] = {qMetaTypeId<IndexList>(), qMetaTypeId<IndexList>(), qMetaTypeId<QVector<int> >()};
    const int s4[] = {qMetaTypeId<IndexList>(), qMetaTypeId<int>(), qMetaTypeId<int>()};
    const int s5[] = {qMetaTypeId<IndexList>(), qMetaTypeId<int>(), qMetaTypeId<int>()};
    const int s6[] = {qMetaTypeId<IndexList>(), qMetaTypeId<int>(), qMetaTypeId<int>(), qMetaTypeId<IndexList>(),qMetaTypeId<int>()};
    const int s7[] = {qMetaTypeId<IndexList>(), qMetaTypeId<IndexList>()};
    const int s9[] = {qMetaTypeId<Qt::Orientation>(), qMetaTypeId<int>(), qMetaTypeId<int>()};
    const int * const signalArgTypes[] = {Q_NULLPTR, s3, s4, s5, s6, s7, Q_NULLPTR, s9};

}

template <class ObjectType, class AdapterType>
struct QAbstractItemAdapterSourceAPI : public SourceApiMap
{
    QAbstractItemAdapterSourceAPI(const QString &name)
        : SourceApiMap()
        , m_name(name)
    {
        _properties[0] = 1;
        _properties[1] = qtro_prop_index<AdapterType>(&AdapterType::availableRoles, static_cast<QVector<int> (QObject::*)()>(0),"availableRoles");
        _signals[0] = 8;
        _signals[1] = qtro_signal_index<AdapterType>(&AdapterType::availableRolesChanged, static_cast<void (QObject::*)()>(0),signalArgCount+0,signalArgTypes[0]);
        _signals[2] = qtro_signal_index<AdapterType>(&AdapterType::dataChanged, static_cast<void (QObject::*)(IndexList,IndexList,QVector<int>)>(0),signalArgCount+1,signalArgTypes[1]);
        _signals[3] = qtro_signal_index<AdapterType>(&AdapterType::rowsInserted, static_cast<void (QObject::*)(IndexList,int,int)>(0),signalArgCount+2,signalArgTypes[2]);
        _signals[4] = qtro_signal_index<AdapterType>(&AdapterType::rowsRemoved, static_cast<void (QObject::*)(IndexList,int,int)>(0),signalArgCount+3,signalArgTypes[3]);
        _signals[5] = qtro_signal_index<AdapterType>(&AdapterType::rowsMoved, static_cast<void (QObject::*)(IndexList,int,int,IndexList,int)>(0),signalArgCount+4,signalArgTypes[4]);
        _signals[6] = qtro_signal_index<AdapterType>(&AdapterType::currentChanged, static_cast<void (QObject::*)(IndexList,IndexList)>(0),signalArgCount+5,signalArgTypes[5]);
        _signals[7] = qtro_signal_index<ObjectType>(&ObjectType::modelReset, static_cast<void (QObject::*)()>(0),signalArgCount+6,signalArgTypes[6]);
        _signals[8] = qtro_signal_index<ObjectType>(&ObjectType::headerDataChanged, static_cast<void (QObject::*)(Qt::Orientation,int,int)>(0),signalArgCount+7,signalArgTypes[7]);
        _methods[0] = 3;
        _methods[1] = qtro_method_index<AdapterType>(&AdapterType::replicaRowRequest, static_cast<void (QObject::*)(IndexList,IndexList,QVector<int>)>(0),"replicaRowRequest(IndexList,IndexList,QVector<int>)",methodArgCount+0,methodArgTypes[0]);
        _methods[2] = qtro_method_index<AdapterType>(&AdapterType::replicaHeaderRequest, static_cast<void (QObject::*)(QVector<Qt::Orientation>,QVector<int>,QVector<int>)>(0),"replicaHeaderRequest(QVector<Qt::Orientation>,QVector<int>,QVector<int>)",methodArgCount+1,methodArgTypes[1]);
        _methods[3] = qtro_method_index<AdapterType>(&AdapterType::replicaSetCurrentIndex, static_cast<void (QObject::*)(IndexList,QItemSelectionModel::SelectionFlags)>(0),"replicaSetCurrentIndex(IndexList,QItemSelectionModel::SelectionFlags)",methodArgCount+2,methodArgTypes[2]);
    }

    QString name() const Q_DECL_OVERRIDE { return m_name; }
    int propertyCount() const Q_DECL_OVERRIDE { return _properties[0]; }
    int signalCount() const Q_DECL_OVERRIDE { return _signals[0]; }
    int methodCount() const Q_DECL_OVERRIDE { return _methods[0]; }
    int sourcePropertyIndex(int index) const Q_DECL_OVERRIDE
    {
        if (index < 0 || index >= _properties[0])
            return -1;
        return _properties[index+1];
    }
    int sourceSignalIndex(int index) const Q_DECL_OVERRIDE
    {
        if (index < 0 || index >= _signals[0])
            return -1;
        return _signals[index+1];
    }
    int sourceMethodIndex(int index) const Q_DECL_OVERRIDE
    {
        if (index < 0 || index >= _methods[0])
            return -1;
        return _methods[index+1];
    }
    int signalParameterCount(int index) const Q_DECL_OVERRIDE { return signalArgCount[index]; }
    int signalParameterType(int sigIndex, int paramIndex) const Q_DECL_OVERRIDE { return signalArgTypes[sigIndex][paramIndex]; }
    int methodParameterCount(int index) const Q_DECL_OVERRIDE { return methodArgCount[index]; }
    int methodParameterType(int methodIndex, int paramIndex) const Q_DECL_OVERRIDE { return methodArgTypes[methodIndex][paramIndex]; }
    int propertyIndexFromSignal(int index) const Q_DECL_OVERRIDE
    {
        switch (index) {
        case 0: return _properties[1];
        }
        return -1;
    }
    const QByteArray signalSignature(int index) const Q_DECL_OVERRIDE
    {
        switch (index) {
        case 0: return QByteArrayLiteral("availableRolesChanged()");
        case 1: return QByteArrayLiteral("dataChanged(IndexList,IndexList,QVector<int>)");
        case 2: return QByteArrayLiteral("rowsInserted(IndexList,int,int)");
        case 3: return QByteArrayLiteral("rowsRemoved(IndexList,int,int)");
        case 4: return QByteArrayLiteral("rowsMoved(IndexList,int,int,IndexList,int)");
        case 5: return QByteArrayLiteral("currentChanged(IndexList,IndexList)");
        case 6: return QByteArrayLiteral("resetModel()");
        case 7: return QByteArrayLiteral("headerDataChanged(Qt::Orientation,int,int)");
        }
        return QByteArrayLiteral("");
    }
    const QByteArray methodSignature(int index) const Q_DECL_OVERRIDE
    {
        switch (index) {
        case 0: return QByteArrayLiteral("replicaRowRequest(IndexList,IndexList,QVector<int>)");
        case 1: return QByteArrayLiteral("replicaHeaderRequest(QVector<Qt::Orientation>,QVector<int>,QVector<int>)");
        case 2: return QByteArrayLiteral("replicaSetCurrentIndex(IndexList,QItemSelectionModel::SelectionFlags)");
        }
        return QByteArrayLiteral("");
    }
    QMetaMethod::MethodType methodType(int) const Q_DECL_OVERRIDE
    {
        return QMetaMethod::Slot;
    }
    const QByteArray typeName(int index) const Q_DECL_OVERRIDE
    {
        switch (index) {
        case 0: return QByteArrayLiteral("DataEntries");
        case 1: return QByteArrayLiteral("QVariantList");
        case 2: return QByteArrayLiteral("");
        }
        return QByteArrayLiteral("");
    }
    bool isAdapterSignal(int index) const Q_DECL_OVERRIDE
    {
        switch (index) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            return true;
        }
        return false;
    }
    bool isAdapterMethod(int index) const Q_DECL_OVERRIDE
    {
        switch (index) {
        case 0:
        case 1:
        case 2:
            return true;
        }
        return false;
    }
    bool isAdapterProperty(int index) const Q_DECL_OVERRIDE
    {
        switch (index) {
        case 0:
            return true;
        }
        return false;
    }

    int _properties[2];
    int _signals[9];
    int _methods[4];
    int signalArgCount[8];
    int methodArgCount[3];
    const int* methodArgTypes[3];
    QString m_name;
};

#endif //QREMOTEOBJECTS_ABSTRACT_ITEM_ADAPTER_P_H
