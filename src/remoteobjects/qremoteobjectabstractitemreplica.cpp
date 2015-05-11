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

#include "qremoteobjectabstractitemreplica.h"
#include "qremoteobjectabstractitemreplica_p.h"

#include "qremoteobjectnode.h"

#include <QDebug>
#include <QRect>
#include <QPoint>

QT_BEGIN_NAMESPACE

inline QDebug operator<<(QDebug stream, const RequestedData &data)
{
    return stream.nospace() << "RequestedData[start=" << data.start << ", end=" << data.end << ", roles=" << data.roles << "]";
}

QAbstractItemReplicaPrivate::QAbstractItemReplicaPrivate()
    : QRemoteObjectReplica()
    , m_selectionModel(0)
    , m_lastRequested(-1)
{
    registerTypes();

    connect(this, &QAbstractItemReplicaPrivate::dataChanged, this, &QAbstractItemReplicaPrivate::onDataChanged);
    connect(this, &QAbstractItemReplicaPrivate::rowsInserted, this, &QAbstractItemReplicaPrivate::onRowsInserted);
    connect(this, &QAbstractItemReplicaPrivate::rowsRemoved, this, &QAbstractItemReplicaPrivate::onRowsRemoved);
    connect(this, &QAbstractItemReplicaPrivate::rowsMoved, this, &QAbstractItemReplicaPrivate::onRowsMoved);
    connect(this, &QAbstractItemReplicaPrivate::currentChanged, this, &QAbstractItemReplicaPrivate::onCurrentChanged);
    connect(this, &QAbstractItemReplicaPrivate::modelReset, this, &QAbstractItemReplicaPrivate::onModelReset);
    connect(this, &QAbstractItemReplicaPrivate::headerDataChanged, this, &QAbstractItemReplicaPrivate::onHeaderDataChanged);
}

QAbstractItemReplicaPrivate::~QAbstractItemReplicaPrivate()
{
}

void QAbstractItemReplicaPrivate::initialize()
{
    QVariantList properties;
    properties << QVariant::fromValue(QVector<int>());
    properties << QVariant::fromValue(QIntHash());
    setProperties(properties);
}

void QAbstractItemReplicaPrivate::registerTypes()
{
    static bool alreadyRegistered = false;
    if (!alreadyRegistered) {
        alreadyRegistered = true;
        qRegisterMetaType<Qt::Orientation>();
        qRegisterMetaType<QVector<Qt::Orientation> >();
        qRegisterMetaTypeStreamOperators<ModelIndex>();
        qRegisterMetaTypeStreamOperators<IndexList>();
        qRegisterMetaTypeStreamOperators<DataEntries>();
        qRegisterMetaTypeStreamOperators<Qt::Orientation>();
        qRegisterMetaTypeStreamOperators<QVector<Qt::Orientation> >();
        qRegisterMetaTypeStreamOperators<QItemSelectionModel::SelectionFlags>();
        qRegisterMetaType<QItemSelectionModel::SelectionFlags>();
        qRegisterMetaType<QIntHash>();
        qRegisterMetaTypeStreamOperators<QIntHash>();
    }
}

inline void removeIndexFromRow(const QModelIndex &index, const QVector<int> &roles, CachedRowEntry *entry)
{
    CachedRowEntry &entryRef = *entry;
    if (index.column() < entryRef.size()) {
        CacheEntry &entry = entryRef[index.column()];
        if (roles.isEmpty()) {
            entry.data.clear();
        } else {
            Q_FOREACH (int role, roles) {
                entry.data.remove(role);
            }
        }
    }
}

struct OnConnectHandler
{
    OnConnectHandler(QAbstractItemReplicaPrivate *rep) : m_rep(rep){}
    void operator()(const QModelIndex &current, const QModelIndex &/*previous*/)
    {
        IndexList currentIndex = toModelIndexList(current, m_rep->q);
        qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "current=" << currentIndex;
        m_rep->replicaSetCurrentIndex(currentIndex, QItemSelectionModel::Clear|QItemSelectionModel::Select|QItemSelectionModel::Current);
    }

    QAbstractItemReplicaPrivate *m_rep;
};

void QAbstractItemReplicaPrivate::setModel(QAbstractItemReplica *model)
{
    q = model;
    setParent(model);
    m_selectionModel.reset(new QItemSelectionModel(model));
    connect(m_selectionModel.data(), &QItemSelectionModel::currentChanged, this, OnConnectHandler(this));
}

void QAbstractItemReplicaPrivate::clearCache(const IndexList &start, const IndexList &end, const QVector<int> &roles = QVector<int>())
{
    Q_ASSERT(start.size() == end.size());

    const QModelIndex startIndex = toQModelIndex(start, q);
    const QModelIndex endIndex = toQModelIndex(end, q);
    Q_ASSERT(startIndex.isValid());
    Q_ASSERT(endIndex.isValid());
    Q_ASSERT(startIndex.parent() == endIndex.parent());
    QModelIndex parentIndex = startIndex.parent();
    CacheData *parentItem = cacheData(parentIndex);

    const int startRow = start.last().row;
    const int lastRow = end.last().row;
    const int startColumn = start.last().column;
    const int lastColumn = end.last().column;
    for (int row = startRow; row <= lastRow; ++row) {
        Q_ASSERT_X(row >= 0 && row < parentItem->children.count(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(row).arg(parentItem->children.count())));
        for (int column = startColumn; column <= lastColumn; ++column) {
            CachedRowEntry *entry = &(parentItem->children[row]->cachedRowEntry);
            removeIndexFromRow(q->index(row, column, parentIndex), roles, entry);
        }
    }
}

void QAbstractItemReplicaPrivate::onDataChanged(const IndexList &start, const IndexList &end, const QVector<int> &roles)
{
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "start=" << start << "end=" << end << "roles=" << roles;

    // we need to clear the cache to make sure the new remote data is fetched if the new data call is happening
    RequestedData data;
    data.start = start;
    data.end = end;
    data.roles = roles;
    clearCache(start, end, roles);
    m_requestedData.append(data);
    QMetaObject::invokeMethod(this, "fetchPendingData", Qt::QueuedConnection);
}

void QAbstractItemReplicaPrivate::onRowsInserted(const IndexList &parent, int start, int end)
{
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "start=" << start << "end=" << end << "parent=" << parent;

    bool treeFullyLazyLoaded = true;
    const QModelIndex parentIndex = toQModelIndex(parent, q, &treeFullyLazyLoaded);
    if (!treeFullyLazyLoaded)
        return;

    CacheData *parentItem = cacheData(parentIndex);
    q->beginInsertRows(parentIndex, start, end);
    parentItem->insertChildren(start, end);
    q->endInsertRows();

    // This is needed to trigger a new data() call to (re-)fetch the actual content
    // of the newly added items.
    const int rowCount = q->rowCount(parentIndex);
    const int columnCount = q->columnCount(parentIndex);
    Q_ASSERT_X(rowCount > 0 && columnCount > 0, __FUNCTION__, qPrintable(QString(QLatin1String("rowCount=%1 columnCount=%2")).arg(rowCount).arg(columnCount)));
    const QModelIndex startIndex = q->index(start, 0, parentIndex);
    const QModelIndex endIndex = q->index(rowCount-1, columnCount-1, parentIndex);
    Q_ASSERT_X(startIndex.isValid() && endIndex.isValid(), __FUNCTION__, qPrintable(QString(QLatin1String("startIndex.isValid=%1 endIndex.isValid=%2")).arg(startIndex.isValid()).arg(endIndex.isValid())));
    emit q->dataChanged(startIndex, endIndex);
}

void QAbstractItemReplicaPrivate::onRowsRemoved(const IndexList &parent, int start, int end)
{
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "start=" << start << "end=" << end << "parent=" << parent;

    bool treeFullyLazyLoaded = true;
    const QModelIndex parentIndex = toQModelIndex(parent, q, &treeFullyLazyLoaded);
    if (!treeFullyLazyLoaded)
        return;

    CacheData *parentItem = cacheData(parentIndex);
    q->beginRemoveRows(parentIndex, start, end);
    parentItem->removeChildren(start, end);
    q->endRemoveRows();
}

void QAbstractItemReplicaPrivate::onRowsMoved(IndexList srcParent, int srcRow, int count, IndexList destParent, int destRow)
{
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO;

    const QModelIndex sourceParent = toQModelIndex(srcParent, q);
    const QModelIndex destinationParent = toQModelIndex(destParent, q);
    Q_ASSERT(!sourceParent.isValid());
    Q_ASSERT(!destinationParent.isValid());
    q->beginMoveRows(sourceParent, srcRow, count, destinationParent, destRow);
//TODO misses parents...
    IndexList start, end;
    start << ModelIndex(srcRow, 0);
    end << ModelIndex(srcRow + count, q->columnCount(sourceParent)-1);
    clearCache(start, end);
    IndexList start2, end2;
    start2 << ModelIndex(destRow, 0);
    end2 << ModelIndex(destRow + count, q->columnCount(destinationParent)-1);
    clearCache(start2, end2);
    q->endMoveRows();
}

void QAbstractItemReplicaPrivate::onCurrentChanged(IndexList current, IndexList previous)
{
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "current=" << current << "previous=" << previous;
    Q_UNUSED(previous);
    Q_ASSERT(m_selectionModel);
    const QModelIndex currentIndex = toQModelIndex(current, q);
    m_selectionModel->setCurrentIndex(currentIndex, QItemSelectionModel::Clear|QItemSelectionModel::Select|QItemSelectionModel::Current);
}

void QAbstractItemReplicaPrivate::handleInitDone(QRemoteObjectPendingCallWatcher *watcher)
{
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO;

    handleModelResetDone(watcher);

    emit q->initialized();
}

void QAbstractItemReplicaPrivate::handleModelResetDone(QRemoteObjectPendingCallWatcher *watcher)
{
    const QSize size = watcher->returnValue().value<QSize>();
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "size=" << size;

    q->beginResetModel();
    m_rootItem.clear();
    if (size.height() > 0)
        m_rootItem.insertChildren(0, size.height() - 1);
    m_rootItem.columnCount = size.width();
    m_headerData[0].resize(size.width());
    m_headerData[1].resize(size.height());
    q->endResetModel();
}

void QAbstractItemReplicaPrivate::handleSizeDone(QRemoteObjectPendingCallWatcher *watcher)
{
    SizeWatcher *sizeWatcher = static_cast<SizeWatcher*>(watcher);
    const QSize size = sizeWatcher->returnValue().value<QSize>();
    CacheData *parentItem = cacheData(sizeWatcher->parentList);
    const QModelIndex parent = toQModelIndex(sizeWatcher->parentList, q);
    if (parentItem->children.isEmpty()) {
        if (size.height() > 0) {
            q->beginInsertRows(parent, 0, size.height() - 1);
            parentItem->insertChildren(0, size.height() - 1);
            q->endInsertRows();
        }
    } else {
        Q_ASSERT_X(parentItem->children.count() == size.height(), __FUNCTION__, qPrintable(QString(QLatin1String("%1 != %2")).arg(parentItem->children.count()).arg(size.height())));
    }

    if (size.width() != m_rootItem.columnCount) {
        const int columnCount = std::max(0, m_rootItem.columnCount);
        m_rootItem.columnCount = size.width();
        if (size.width() > columnCount) {
            Q_ASSERT(size.width() > 0);
            q->beginInsertColumns(parent, columnCount, size.width() - 1);
            q->endInsertColumns();
        } else {
            Q_ASSERT_X(size.width() == columnCount, __FUNCTION__, qPrintable(QString(QLatin1String("%1 != %2")).arg(size.width()).arg(columnCount)));
        }
    }
}

void QAbstractItemReplicaPrivate::init()
{
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO;
    SizeWatcher *watcher = doModelReset();
    connect(watcher, &SizeWatcher::finished, this, &QAbstractItemReplicaPrivate::handleInitDone);
}

SizeWatcher* QAbstractItemReplicaPrivate::doModelReset()
{
    IndexList parentList;
    QRemoteObjectPendingReply<QSize> reply = replicaSizeRequest(parentList);
    SizeWatcher *watcher = new SizeWatcher(parentList, reply);
    return watcher;
}

inline void fillCacheEntry(CacheEntry *entry, const IndexValuePair &pair, const QVector<int> &roles)
{
    Q_ASSERT(entry);

    const QVariantList &data = pair.data;
    Q_ASSERT(roles.size() == data.size());

    entry->flags = pair.flags;

    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "data.size=" << data.size();
    for (int i = 0; i < data.size(); ++i) {
        const int role = roles[i];
        const QVariant dataVal = data[i];
        qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "role=" << role << "data=" << dataVal;
        entry->data[role] = dataVal;
    }
}

inline void fillRow(CacheData *item, const IndexValuePair &pair, const QAbstractItemModel *model, const QVector<int> &roles)
{
    CachedRowEntry &rowRef = item->cachedRowEntry;
    const QModelIndex index = toQModelIndex(pair.index, model);
    Q_ASSERT(index.isValid());
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "row=" << index.row() << "column=" << index.column();
    if (index.column() == 0)
        item->hasChildren = pair.hasChildren;
    bool existed = false;
    for (int i = 0; i < rowRef.size(); ++i) {
        if (i == index.column()) {
            fillCacheEntry(&rowRef[i], pair, roles);
            existed = true;
        }
    }
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "existed=" << existed;
    if (!existed) {
        CacheEntry entries;
        fillCacheEntry(&entries, pair, roles);
        rowRef.append(entries);
    }
}

int collectEntriesForRow(DataEntries* filteredEntries, int row, const DataEntries &entries, int startIndex)
{
    Q_ASSERT(filteredEntries);
    const int size = entries.data.size();
    for (int i = startIndex; i < size; ++i)
    {
        const IndexValuePair &pair = entries.data[i];
        if (pair.index.last().row == row)
            filteredEntries->data << pair;
        else
            return i;
    }
    return size;
}

void QAbstractItemReplicaPrivate::requestedData(QRemoteObjectPendingCallWatcher *qobject)
{
    RowWatcher *watcher = static_cast<RowWatcher *>(qobject);
    Q_ASSERT(watcher);
    Q_ASSERT(watcher->start.size() == watcher->end.size());

    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "start=" << watcher->start << "end=" << watcher->end;

    IndexList parentList = watcher->start;
    Q_ASSERT(!parentList.isEmpty());
    parentList.pop_back();
    CacheData *parentItem = cacheData(parentList);
    DataEntries entries = watcher->returnValue().value<DataEntries>();

    const int rowCount = parentItem->children.count();
    const int columnCount = parentItem->columnCount;

    if (rowCount < 1 || columnCount < 1)
        return;

    const int startRow = watcher->start.last().row;
    const int endRow = std::min(watcher->end.last().row, rowCount - 1);
    const int startColumn = watcher->start.last().column;
    const int endColumn = std::min(watcher->end.last().column, columnCount - 1);
    Q_ASSERT_X(startRow >= 0 && startRow < parentItem->children.size(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(startRow).arg(parentItem->children.size())));
    Q_ASSERT_X(endRow >= 0 && endRow < parentItem->children.size(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(endRow).arg(parentItem->children.size())));

    for (int i = 0; i < entries.data.size(); ++i) {
        IndexValuePair pair = entries.data[i];
        CacheData *item = cacheData(pair.index);
        fillRow(item, pair, q, watcher->roles);
    }

    const QModelIndex parentIndex = toQModelIndex(parentList, q);
    const QModelIndex startIndex = q->index(startRow, startColumn, parentIndex);
    const QModelIndex endIndex = q->index(endRow, endColumn, parentIndex);
    Q_ASSERT(startIndex.isValid());
    Q_ASSERT(endIndex.isValid());
    emit q->dataChanged(startIndex, endIndex, watcher->roles);
}

void QAbstractItemReplicaPrivate::fetchPendingData()
{
    if (m_requestedData.isEmpty())
        return;

    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "m_requestedData.size=" << m_requestedData.size();

    QVector<RequestedData> finalRequests;
    RequestedData curData;
    Q_FOREACH (const RequestedData &data, m_requestedData) {
        qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "REQUESTED start=" << data.start << "end=" << data.end << "roles=" << data.roles;

        Q_ASSERT(!data.start.isEmpty());
        Q_ASSERT(!data.end.isEmpty());
        Q_ASSERT(data.start.size() == data.end.size());
        if (curData.start.isEmpty() || curData.start.last().row == -1 || curData.start.last().column == -1)
            curData = data;
        if (curData.start.size() != data.start.size()) {
            finalRequests.push_back(curData);
            curData = data;
        } else {
            if (data.start.size() > 1) {
                for (int i = 0; i < data.start.size() - 1; ++i) {
                    if (curData.start[i].row != data.start[i].row ||
                        curData.start[i].column != data.start[i].column) {
                        finalRequests.push_back(curData);
                        curData = data;
                    }
                }
            }

            const ModelIndex curIndStart = curData.start.last();
            const ModelIndex curIndEnd = curData.end.last();
            const ModelIndex dataIndStart = data.start.last();
            const ModelIndex dataIndEnd = data.end.last();
            const ModelIndex resStart(std::min(curIndStart.row, dataIndStart.row), std::min(curIndStart.column, dataIndStart.column));
            const ModelIndex resEnd(std::max(curIndEnd.row, dataIndEnd.row), std::max(curIndEnd.column, dataIndEnd.column));
            QVector<int> roles = curData.roles;
            if (!curData.roles.isEmpty())
                Q_FOREACH (int role, data.roles)
                    if (!curData.roles.contains(role))
                        roles.append(role);
            QRect firstRect( QPoint(curIndStart.row, curIndStart.column), QPoint(curIndEnd.row, curIndEnd.column));
            QRect secondRect( QPoint(dataIndStart.row, dataIndStart.column), QPoint(dataIndEnd.row, dataIndEnd.column));

            const bool borders = (qAbs(curIndStart.row - dataIndStart.row) == 1) ||
                                 (qAbs(curIndStart.column - dataIndStart.column) == 1) ||
                                 (qAbs(curIndEnd.row - dataIndEnd.row) == 1) ||
                                 (qAbs(curIndEnd.column - dataIndEnd.column) == 1);
            if (firstRect.intersects(secondRect) || borders) {
                IndexList start = curData.start;
                start.pop_back();
                start.push_back(resStart);
                IndexList end = curData.end;
                end.pop_back();
                end.push_back(resEnd);
                curData.start = start;
                curData.end = end;
                curData.roles = roles;
                Q_ASSERT(!start.isEmpty());
                Q_ASSERT(!end.isEmpty());
            } else {
                finalRequests.push_back(curData);
                curData = data;
            }
        }
    }
    finalRequests.push_back(curData);
    //qCDebug(QT_REMOTEOBJECT_MODELS) << "Final requests" << finalRequests;
    Q_FOREACH (const RequestedData &data, finalRequests) {
        qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO << "FINAL start=" << data.start << "end=" << data.end << "roles=" << data.roles;

        QRemoteObjectPendingReply<DataEntries> reply = replicaRowRequest(data.start, data.end, data.roles);
        RowWatcher *watcher = new RowWatcher(data.start, data.end, data.roles, reply);
        connect(watcher, &RowWatcher::finished, this, &QAbstractItemReplicaPrivate::requestedData);
    }
    m_requestedData.clear();
}

void QAbstractItemReplicaPrivate::onModelReset()
{
    qCDebug(QT_REMOTEOBJECT_MODELS) << Q_FUNC_INFO;
    SizeWatcher *watcher = doModelReset();
    connect(watcher, &SizeWatcher::finished, this, &QAbstractItemReplicaPrivate::handleModelResetDone);
}

void QAbstractItemReplicaPrivate::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    // TODO clean cache
    const int index = orientation == Qt::Horizontal ? 0 : 1;
    QVector<CacheEntry> &entries = m_headerData[index];
    for (int i = first; i < last; ++i )
        entries[i].data.clear();
    emit q->headerDataChanged(orientation, first, last);
}

void QAbstractItemReplicaPrivate::fetchPendingHeaderData()
{
    QVector<int> roles;
    QVector<int> sections;
    QVector<Qt::Orientation> orientations;
    Q_FOREACH (const RequestedHeaderData &data, m_requestedHeaderData) {
        roles.push_back(data.role);
        sections.push_back(data.section);
        orientations.push_back(data.orientation);
    }
    QRemoteObjectPendingReply<QVariantList> reply = replicaHeaderRequest(orientations, sections, roles);
    HeaderWatcher *watcher = new HeaderWatcher(orientations, sections, roles, reply);
    connect(watcher, &HeaderWatcher::finished, this, &QAbstractItemReplicaPrivate::requestedHeaderData);
    m_requestedHeaderData.clear();
}

static inline QVector<QPair<int, int> > listRanges(const QVector<int> &list)
{
    QVector<QPair<int, int> > result;
    if (!list.isEmpty()) {
        QPair<int, int> currentElem = qMakePair(list.first(), list.first());
        QVector<int>::const_iterator end = list.end();
        for (QVector<int>::const_iterator it = list.constBegin() + 1; it != end; ++it) {
            if (currentElem.first == *it +1)
                currentElem.first = *it;
            else if ( currentElem.second == *it -1)
                currentElem.second = *it;
            else if (currentElem.first <= *it && currentElem.second >= *it)
                continue;
            else {
                result.push_back(currentElem);
                currentElem.first = *it;
                currentElem.second = *it;
            }
        }
        result.push_back(currentElem);
    }
    return result;
}

void QAbstractItemReplicaPrivate::requestedHeaderData(QRemoteObjectPendingCallWatcher *qobject)
{
    HeaderWatcher *watcher = static_cast<HeaderWatcher *>(qobject);
    Q_ASSERT(watcher);

    QVariantList data = watcher->returnValue().value<QVariantList>();
    Q_ASSERT(watcher->orientations.size() == data.size());
    Q_ASSERT(watcher->sections.size() == data.size());
    Q_ASSERT(watcher->roles.size() == data.size());
    QVector<int> horizontalSections;
    QVector<int> verticalSections;

    for (int i = 0; i < data.size(); ++i) {
        if (watcher->orientations[i] == Qt::Horizontal)
            horizontalSections.append(watcher->sections[i]);
        else
            verticalSections.append(watcher->sections[i]);
        const int index = watcher->orientations[i] == Qt::Horizontal ? 0 : 1;
        const int role = watcher->roles[i];
        QMap<int, QVariant> &dat = m_headerData[index][watcher->sections[i]].data;
        dat[role] = data[i];
    }
    QVector<QPair<int, int> > horRanges = listRanges(horizontalSections);
    QVector<QPair<int, int> > verRanges = listRanges(verticalSections);

    for (int i = 0; i < horRanges.size(); ++i)
        emit q->headerDataChanged(Qt::Horizontal, horRanges[i].first, horRanges[i].second);
    for (int i = 0; i < verRanges.size(); ++i)
        emit q->headerDataChanged(Qt::Vertical, verRanges[i].first, verRanges[i].second);
}

QAbstractItemReplica::QAbstractItemReplica(QAbstractItemReplicaPrivate *rep)
    : QAbstractItemModel()
    , d(rep)
{
    rep->setModel(this);

    connect(rep, &QAbstractItemReplicaPrivate::initialized, d.data(), &QAbstractItemReplicaPrivate::init);
}

QAbstractItemReplica::~QAbstractItemReplica()
{
}

QVariant findData(const CachedRowEntry &row, const QModelIndex &index, int role, bool *cached = 0)
{
    if (index.column() < row.size()) {
        const CacheEntry &entry = row[index.column()];
        QMap<int, QVariant>::ConstIterator it = entry.data.constFind(role);
        if (it != entry.data.constEnd()) {
            if (cached)
                *cached = true;
            return it.value();
        }
    }
    if (cached)
        *cached = false;
    return QVariant();
}

int findCacheBorder(int start, int range, CacheData* data) {
    const int limit = std::min(std::max(start + range, 0), data->children.size());
    const int step = limit < start ? -1 : 1;
    for (int i = start + step; i < limit; i += step) {
        const CachedRowEntry &entry = data->children[i]->cachedRowEntry;
        if (!entry.isEmpty() || i < 0 || i > data->children.size())
            return i - step;
    }
    return start;
}

QItemSelectionModel* QAbstractItemReplica::selectionModel() const
{
    return d->m_selectionModel.data();
}

QVariant QAbstractItemReplica::data(const QModelIndex & index, int role) const
{
    Q_ASSERT(index.isValid());

    if (!d->isInitialized()) {
        qCDebug(QT_REMOTEOBJECT_MODELS)<<"Data not initialized yet";
        return QVariant();
    }

    CacheData *item = d->cacheData(index);
    CacheData *parentItem = item->parent;
    Q_ASSERT(parentItem);
    Q_ASSERT(parentItem->children.indexOf(item) >= 0);
    Q_ASSERT(index.row() < parentItem->children.count());

    bool cached = false;
    const CachedRowEntry &entry = item->cachedRowEntry;
    QVariant result = findData(entry, index, role, &cached);
    if (!cached) {
        const int row = index.row();
        const int low = findCacheBorder(row, -HalfLookAhead, parentItem);
        const int high = findCacheBorder(row, HalfLookAhead, parentItem);
        Q_ASSERT_X(low >= 0 && low < parentItem->children.size(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(low).arg(parentItem->children.size())));
        Q_ASSERT_X(high >= 0 && high < parentItem->children.size(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(high).arg(parentItem->children.size())));

        IndexList parentList = toModelIndexList(index.parent(), this);
        IndexList start = IndexList() << parentList << ModelIndex(low, 0);
        IndexList end = IndexList() << parentList << ModelIndex(high, std::max(0, parentItem->columnCount - 1));
        Q_ASSERT(toQModelIndex(start, this).isValid());

        RequestedData data;
        QVector<int> roles;
        roles << role;
        data.start = start;
        data.end = end;
        data.roles = roles;
        d->m_requestedData.push_back(data);
        qCDebug(QT_REMOTEOBJECT_MODELS) << "FETCH PENDING DATA" << start << end << roles;
        QMetaObject::invokeMethod(d.data(), "fetchPendingData", Qt::QueuedConnection);
    }
    return result;
}
QModelIndex QAbstractItemReplica::parent(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());
    CacheData* item = d->cacheData(index);
    Q_ASSERT(item != &d->m_rootItem);
    Q_ASSERT(item->parent);
    if (item->parent == &d->m_rootItem)
        return QModelIndex();
    Q_ASSERT(item->parent->parent);
    int row = item->parent->parent->children.indexOf(item->parent);
    Q_ASSERT(row >= 0);
    return createIndex(row, 0, item->parent);
}
QModelIndex QAbstractItemReplica::index(int row, int column, const QModelIndex &parent) const
{
    CacheData *parentItem = d->cacheData(parent);
    Q_ASSERT_X((parent.isValid() && parentItem != &d->m_rootItem) || (!parent.isValid() && parentItem == &d->m_rootItem), __FUNCTION__, qPrintable(QString(QLatin1String("isValid=%1 equals=%2")).arg(parent.isValid()).arg(parentItem == &d->m_rootItem)));

    // hmpf, following works around a Q_ASSERT-bug in QAbstractItemView::setModel which does just call
    // d->model->index(0,0) without checking the range before-hand what triggers our assert in case the
    // model is empty when view::setModel is called :-/ So, work around with the following;
    if (row < 0 || row >= parentItem->children.count())
        return QModelIndex();
    if (column < 0 || column >= parentItem->columnCount)
        return QModelIndex();

    Q_ASSERT_X(row >= 0 && row < parentItem->children.count(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(row).arg(parentItem->children.count())));

    return createIndex(row, column, parentItem->children[row]);
}
bool QAbstractItemReplica::hasChildren(const QModelIndex &parent) const
{
    CacheData *parentItem = d->cacheData(parent);
    return parentItem->hasChildren;
}
int QAbstractItemReplica::rowCount(const QModelIndex &parent) const
{
    CacheData *parentItem = d->cacheData(parent);
    if (parentItem->hasChildren && parentItem->children.isEmpty()) {
        IndexList parentList = toModelIndexList(parent, this);
        QRemoteObjectPendingReply<QSize> reply = d->replicaSizeRequest(parentList);
        SizeWatcher *watcher = new SizeWatcher(parentList, reply);
        connect(watcher, &SizeWatcher::finished, d.data(), &QAbstractItemReplicaPrivate::handleSizeDone);
    }
    return parentItem->children.count();
}
int QAbstractItemReplica::columnCount(const QModelIndex &parent) const
{
    CacheData *parentItem = d->cacheData(parent);
    while (parentItem->columnCount < 0 && parentItem->parent)
        parentItem = parentItem->parent;
    return std::max(0, parentItem->columnCount);
}

QVariant QAbstractItemReplica::headerData(int section, Qt::Orientation orientation, int role) const
{
    const int index = orientation == Qt::Horizontal ? 0 : 1;
    const QVector<CacheEntry> elem = d->m_headerData[index];
    if (section >= elem.size())
        return QVariant();

    const QMap<int, QVariant> &dat = elem.at(section).data;
    QMap<int, QVariant>::ConstIterator it = dat.constFind(role);
    if (it != dat.constEnd())
        return it.value();

    RequestedHeaderData data;
    data.role = role;
    data.section = section;
    data.orientation = orientation;
    d->m_requestedHeaderData.push_back(data);
    QMetaObject::invokeMethod(d.data(), "fetchPendingHeaderData", Qt::QueuedConnection);
    return QVariant();
}

Qt::ItemFlags QAbstractItemReplica::flags(const QModelIndex &index) const
{
    CacheEntry *entry = d->cacheEntry(index);
    return entry ? entry->flags : Qt::NoItemFlags;
}

bool QAbstractItemReplica::isInitialized() const
{
    return d->isInitialized();
}

bool QAbstractItemReplica::hasData(const QModelIndex &index, int role) const
{
    if (!d->isInitialized() || !index.isValid())
        return false;
    CacheData *item = d->cacheData(index);
    bool cached = false;
    const CachedRowEntry &entry = item->cachedRowEntry;
    QVariant result = findData(entry, index, role, &cached);
    Q_UNUSED(result);
    return cached;
}

QVector<int> QAbstractItemReplica::availableRoles() const
{
    return d->availableRoles();
}

QHash<int, QByteArray> QAbstractItemReplica::roleNames() const
{
    return d->roleNames();
}

QT_END_NAMESPACE
