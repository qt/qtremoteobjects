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

#ifndef QREMOTEOBJECTS_ABSTRACT_ITEM_REPLICA_P_H
#define QREMOTEOBJECTS_ABSTRACT_ITEM_REPLICA_P_H

#include "qremoteobjectabstractitemmodeltypes.h"
#include "qremoteobjectabstractitemreplica.h"
#include "qremoteobjectreplica.h"
#include "qremoteobjectpendingcall.h"

namespace {
const int BufferSize = 1000;
const int LookAhead = 100;
const int HalfLookAhead = LookAhead/2;
}

struct CacheEntry
{
    QMap<int, QVariant> data;
    Qt::ItemFlags flags;

    explicit CacheEntry()
        : flags(Qt::NoItemFlags)
    {}
};

typedef QVector<CacheEntry> CachedRowEntry;

struct CacheData
{
    CacheData *parent;
    CachedRowEntry cachedRowEntry;

    bool hasChildren;
    QList<CacheData*> children;
    int columnCount;

    explicit CacheData(CacheData *parentItem = 0)
        : parent(parentItem)
        , hasChildren(false)
        , columnCount(0)
    {}
    ~CacheData() { qDeleteAll(children); }
    void insertChildren(int start, int end) {
        Q_ASSERT_X(start >= 0 && start <= end, __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 <= %2")).arg(start).arg(end)));
        for (int i = start; i <= end; ++i)
            children.insert(i, new CacheData(this));
        if (!children.isEmpty())
            hasChildren = true;
    }
    void removeChildren(int start, int end) {
        Q_ASSERT_X(start >= 0 && start <= end && end < children.count(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 <= %2 < %3")).arg(start).arg(end).arg(children.count())));
        for (int i = start; i <= end; ++i)
            delete children.takeAt(start);
        if (children.isEmpty())
            hasChildren = false;
    }
    void clear() {
        cachedRowEntry.clear();
        qDeleteAll(children);
        children.clear();
        hasChildren = false;
        columnCount = 0;
    }

};

struct RequestedData
{
    IndexList start;
    IndexList end;
    QVector<int> roles;
};

struct RequestedHeaderData
{
    int role;
    int section;
    Qt::Orientation orientation;
};

class SizeWatcher : public QRemoteObjectPendingCallWatcher
{
public:
    SizeWatcher(IndexList _parentList, const QRemoteObjectPendingReply<QSize> &reply)
        : QRemoteObjectPendingCallWatcher(reply),
          parentList(_parentList) {}
    IndexList parentList;
};

class RowWatcher : public QRemoteObjectPendingCallWatcher
{
public:
    RowWatcher(IndexList _start, IndexList _end, QVector<int> _roles, const QRemoteObjectPendingReply<DataEntries> &reply)
        : QRemoteObjectPendingCallWatcher(reply),
          start(_start),
          end(_end),
          roles(_roles) {}
    IndexList start, end;
    QVector<int> roles;
};

class HeaderWatcher : public QRemoteObjectPendingCallWatcher
{
public:
    HeaderWatcher(QVector<Qt::Orientation> _orientations, QVector<int> _sections, QVector<int> _roles, const QRemoteObjectPendingReply<QVariantList> &reply)
        : QRemoteObjectPendingCallWatcher(reply),
          orientations(_orientations),
          sections(_sections),
          roles(_roles) {}
    QVector<Qt::Orientation> orientations;
    QVector<int> sections, roles;
};

class QAbstractItemReplicaPrivate : public QRemoteObjectReplica
{
    Q_OBJECT
    //TODO Use an input name for the model on the Replica side
    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, "ServerModelAdapter")
    Q_PROPERTY(QVector<int> availableRoles READ availableRoles NOTIFY availableRolesChanged)
    Q_PROPERTY(QIntHash roleNames READ roleNames)
public:
    QAbstractItemReplicaPrivate();
    ~QAbstractItemReplicaPrivate();
    void initialize();
    void registerTypes();

    QVector<int> availableRoles() const
    {
        return propAsVariant(0).value<QVector<int> >();
    }

    QHash<int, QByteArray> roleNames() const
    {
       QIntHash roles = propAsVariant(1).value<QIntHash>();
       return roles;
    }

    void setModel(QAbstractItemReplica *model);
    bool clearCache(const IndexList &start, const IndexList &end, const QVector<int> &roles);

Q_SIGNALS:
    void availableRolesChanged();
    void dataChanged(IndexList topLeft, IndexList bottomRight, QVector<int> roles);
    void rowsInserted(IndexList parent, int first, int last);
    void rowsRemoved(IndexList parent, int first, int last);
    void rowsMoved(IndexList parent, int start, int end, IndexList destination, int row);
    void currentChanged(IndexList current, IndexList previous);
    void modelReset();
    void headerDataChanged(Qt::Orientation,int,int);
    void columnsInserted(IndexList parent, int first, int last);

public Q_SLOTS:
    QRemoteObjectPendingReply<QSize> replicaSizeRequest(IndexList parentList)
    {
        static int __repc_index = QAbstractItemReplicaPrivate::staticMetaObject.indexOfSlot("replicaSizeRequest(IndexList)");
        QVariantList __repc_args;
        __repc_args << QVariant::fromValue(parentList);
        return QRemoteObjectPendingReply<QSize>(sendWithReply(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args));
    }
    QRemoteObjectPendingReply<DataEntries> replicaRowRequest(IndexList start, IndexList end, QVector<int> roles)
    {
        static int __repc_index = QAbstractItemReplicaPrivate::staticMetaObject.indexOfSlot("replicaRowRequest(IndexList,IndexList,QVector<int>)");
        QVariantList __repc_args;
        __repc_args << QVariant::fromValue(start) << QVariant::fromValue(end) << QVariant::fromValue(roles);
        return QRemoteObjectPendingReply<DataEntries>(sendWithReply(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args));
    }
    QRemoteObjectPendingReply<QVariantList> replicaHeaderRequest(QVector<Qt::Orientation> orientations, QVector<int> sections, QVector<int> roles)
    {
        static int __repc_index = QAbstractItemReplicaPrivate::staticMetaObject.indexOfSlot("replicaHeaderRequest(QVector<Qt::Orientation>,QVector<int>,QVector<int>)");
        QVariantList __repc_args;
        __repc_args << QVariant::fromValue(orientations) << QVariant::fromValue(sections) << QVariant::fromValue(roles);
        return QRemoteObjectPendingReply<QVariantList>(sendWithReply(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args));
    }
    void replicaSetCurrentIndex(IndexList index, QItemSelectionModel::SelectionFlags command)
    {
        static int __repc_index = QAbstractItemReplicaPrivate::staticMetaObject.indexOfSlot("replicaSetCurrentIndex(IndexList,QItemSelectionModel::SelectionFlags)");
        QVariantList __repc_args;
        __repc_args << QVariant::fromValue(index) << QVariant::fromValue(command);
        send(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args);
    }
    void replicaSetData(IndexList index, const QVariant &value, int role)
    {
        static int __repc_index = QAbstractItemReplicaPrivate::staticMetaObject.indexOfSlot("replicaSetData(IndexList,QVariant,int)");
        QVariantList __repc_args;
        __repc_args << QVariant::fromValue(index) << QVariant::fromValue(value) << QVariant::fromValue(role);
        send(QMetaObject::InvokeMetaMethod, __repc_index, __repc_args);
    }
    void onHeaderDataChanged(Qt::Orientation orientation, int first, int last);
    void onDataChanged(const IndexList &start, const IndexList &end, const QVector<int> &roles);
    void onRowsInserted(const IndexList &parent, int start, int end);
    void onRowsRemoved(const IndexList &parent, int start, int end);
    void onColumnsInserted(const IndexList &parent, int start, int end);
    void onRowsMoved(IndexList srcParent, int srcRow, int count, IndexList destParent, int destRow);
    void onCurrentChanged(IndexList current, IndexList previous);
    void onModelReset();
    void requestedData(QRemoteObjectPendingCallWatcher *);
    void requestedHeaderData(QRemoteObjectPendingCallWatcher *);
    void init();
    void fetchPendingData();
    void fetchPendingHeaderData();
    void handleInitDone(QRemoteObjectPendingCallWatcher *watcher);
    void handleModelResetDone(QRemoteObjectPendingCallWatcher *watcher);
    void handleSizeDone(QRemoteObjectPendingCallWatcher *watcher);

public:
    QScopedPointer<QItemSelectionModel> m_selectionModel;
    QVector<CacheEntry> m_headerData[2];

    CacheData m_rootItem;
    inline CacheData* cacheData(const QModelIndex &index) const {
        Q_ASSERT(!index.isValid() || index.model() == q);
        CacheData *data = index.isValid() ? static_cast<CacheData*>(index.internalPointer()) : const_cast<CacheData*>(&m_rootItem);
        Q_ASSERT(data);
        return data;
    }
    inline CacheData* cacheData(const IndexList &index) const {
        return cacheData(toQModelIndex(index, q));
    }
    inline CacheEntry* cacheEntry(const QModelIndex &index) const {
        CacheData *data = cacheData(index);
        if (index.column() < 0 || index.column() >= data->cachedRowEntry.size())
            return 0;
        CacheEntry &entry = data->cachedRowEntry[index.column()];
        return &entry;
    }
    inline CacheEntry* cacheEntry(const IndexList &index) const {
        return cacheEntry(toQModelIndex(index, q));
    }

    SizeWatcher* doModelReset();

    int m_lastRequested;
    QVector<RequestedData> m_requestedData;
    QVector<RequestedHeaderData> m_requestedHeaderData;
    QAbstractItemReplica *q;
};

#endif // QREMOTEOBJECTS_ABSTRACT_ITEM_REPLICA_P_H

