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
    , m_rows(BufferSize)
    , m_lastRequested(-1)
{
    registerTypes();
}

QAbstractItemReplicaPrivate::~QAbstractItemReplicaPrivate()
{
}

void QAbstractItemReplicaPrivate::initialize()
{
    QVariantList properties;
    properties.reserve(3);
    properties << QVariant::fromValue(int());
    properties << QVariant::fromValue(int());
    properties << QVariant::fromValue(QVector<int>());
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
    }
}

inline void removeIndexFromRow(const QModelIndex &index, const QVector<int> &roles, CachedRowEntry *entry)
{
    CachedRowEntry &entryRef = *entry;
    if (index.column() < entryRef.size()) {
        CacheEntry &data = entryRef[index.column()];
        if (roles.isEmpty()) {
            data.clear();
        } else {
            Q_FOREACH (int role, roles) {
                int index = -1;
                for (int j = 0; j < data.size(); ++j)
                    if (data[j].first == role)
                        index = j;
                if (index != -1)
                    data.remove(index);
            }
        }
    }
}

// only table based right now
void QAbstractItemReplicaPrivate::clearCache(const IndexList &start, const IndexList &end, const QVector<int> &roles = QVector<int>())
{
    Q_ASSERT(start.size() == end.size() && start.size() == 1);
    const int startRow = start.first().row;
    const int lastRow = end.first().row;
    const int startColumn = start.first().column;
    const int lastColumn = end.first().column;
    for (int row = startRow; row <= lastRow; ++row) {
        for (int column = startColumn; column <= lastColumn; ++column) {
            removeIndexFromRow(q->index(row, column), roles, &m_rows[row]);
        }
    }
}

void QAbstractItemReplicaPrivate::onDataChanged(const IndexList &start, const IndexList &end, const QVector<int> &roles)
{
    // we need to clear the cache to make sure the new remote data is fetched if the new data call is happening
    RequestedData data;
    data.start = start;
    data.end = end;
    data.roles = roles;
    clearCache(start, end, roles);
    m_requestedData.append(data);
    QMetaObject::invokeMethod(this, "fetchPendingData", Qt::QueuedConnection);
}

//tableModel only right now
void QAbstractItemReplicaPrivate::onRowsInserted(const IndexList &parent, int start, int end)
{
    const QModelIndex index = toQModelIndex(parent, q);
    Q_ASSERT(!index.isValid());
    q->beginInsertRows(index, start, end);
    m_rows.insert(start, end - start + 1, CachedRowEntry());
    q->endInsertRows();
    emit q->dataChanged(q->index(start,0), q->index(rowCount(), columnCount()));
}

//tableModel only right now
void QAbstractItemReplicaPrivate::onRowsRemoved(const IndexList &parent, int start, int end)
{
    const QModelIndex index = toQModelIndex(parent, q);
    Q_ASSERT(!index.isValid());
    q->beginRemoveRows(index, start, end);
    m_rows.remove(start, end - start + 1);
    q->endRemoveRows();
    emit q->dataChanged(q->index(start,0), q->index(rowCount(), columnCount()));
}

void QAbstractItemReplicaPrivate::onRowsMoved(IndexList srcParent, int srcRow, int count, IndexList destParent, int destRow)
{
    const QModelIndex sourceParent = toQModelIndex(srcParent, q);
    const QModelIndex destinationParent = toQModelIndex(destParent, q);
    Q_ASSERT(!sourceParent.isValid());
    Q_ASSERT(!destinationParent.isValid());
    q->beginMoveRows(sourceParent, srcRow, count, destinationParent, destRow);
    IndexList start, end;
    start << ModelIndex(srcRow, 0);
    end << ModelIndex(srcRow + count, columnCount());
    clearCache(start, end);
    IndexList start2, end2;
    start2 << ModelIndex(destRow, 0);
    end2 << ModelIndex(destRow + count, columnCount());
    clearCache(start2, end2);
    q->endMoveRows();
}

void QAbstractItemReplicaPrivate::init()
{
    IndexList start;
    IndexList end;
    start << ModelIndex(0, 0);
    end << ModelIndex(LookAhead, columnCount()-1);
    m_rows.clear();
    m_rows.resize(rowCount());
    m_headerData[0].resize(columnCount());
    m_headerData[1].resize(rowCount());
    QRemoteObjectPendingReply<DataEntries> reply = replicaRowRequest(start, end, availableRoles());
    RowWatcher *watcher = new RowWatcher(start, end, availableRoles(), reply);
    connect(watcher, &RowWatcher::finished, this, &QAbstractItemReplicaPrivate::requestedData);
    connect(watcher, &RowWatcher::finished, q, &QAbstractItemReplica::initialized);
    //Maybe fetch header data as well?
    q->beginInsertRows(QModelIndex(), 0, rowCount() - 1);
    q->endInsertRows();
    q->beginInsertColumns(QModelIndex(), 0, columnCount() - 1);
    q->endInsertColumns();
}

struct CacheRoleComparator
{
    CacheRoleComparator(int role) : m_role(role) {}
    bool operator()(const QPair<int, QVariant> &data)
    {
        return m_role == data.first;
    }

    const int m_role;
};

inline void fillCacheEntry(CacheEntry *entry, const QVariantList &data, const QVector<int> &roles)
{
    Q_ASSERT(entry);
    Q_ASSERT(roles.size() == data.size());
    for (int i = 0; i < data.size(); ++i) {
        const int role = roles[i];
        const QVariant dataVal = data[i];
        CacheEntry::iterator res = std::find_if(entry->begin(), entry->end(), CacheRoleComparator(role));
        if (res != entry->end()) {
            res->second = dataVal;
        } else {
            entry->append(qMakePair(role, dataVal));
        }
    }
}

inline void fillRow(CachedRowEntry *row, const DataEntries &newData, const QAbstractItemModel *model, const QVector<int> &roles)
{
    Q_ASSERT(row);
    CachedRowEntry &rowRef = *row;
    Q_FOREACH (const IndexValuePair &pair, newData) {
        const QModelIndex index = toQModelIndex(pair.first, model);
        bool existed = false;
        for (int i = 0; i < rowRef.size(); ++i) {
            if (i == index.column()) {
                fillCacheEntry(&rowRef[i], pair.second, roles);
                existed = true;
            }
        }
        if (!existed) {
            CacheEntry entries;
            fillCacheEntry(&entries, pair.second, roles);
            rowRef.append(entries);
        }
    }
}

int collectEntriesForRow(DataEntries* filteredEntries, int row, const DataEntries &entries, int startIndex)
{
    Q_ASSERT(filteredEntries);
    for (int i = startIndex; i < entries.size(); ++i)
    {
        const IndexValuePair &pair = entries.at(i);
        if (pair.first.first().row == row)
            filteredEntries->append(pair);
        else
            return i;
    }
    return entries.size();
}

void QAbstractItemReplicaPrivate::requestedData(QRemoteObjectPendingCallWatcher *qobject)
{
    RowWatcher *watcher = static_cast<RowWatcher *>(qobject);
    Q_ASSERT(watcher);
    Q_ASSERT(watcher->start.size() == watcher->end.size());
    const int startRow = watcher->start.last().row;
    const int endRow = qMin(watcher->end.last().row, m_rows.size()-1);
    int startIndex = 0;
    DataEntries data = watcher->returnValue().value<DataEntries>();
    for (int i = startRow; i <= endRow; ++i) {
        DataEntries filteredData;
        startIndex = collectEntriesForRow(&filteredData, i, data, startIndex);
        fillRow(&m_rows[i], filteredData, q, watcher->roles);
    }

    const QModelIndex startQIndex = toQModelIndex(watcher->start, q);
    const QModelIndex endQIndex = toQModelIndex(watcher->end, q);
    emit q->dataChanged(startQIndex, endQIndex, watcher->roles);
}

//TODO this is table optimized
void QAbstractItemReplicaPrivate::fetchPendingData()
{
    if (m_requestedData.isEmpty())
        return;

    QVector<RequestedData> finalRequests;
    RequestedData curData;
    Q_FOREACH (const RequestedData &data, m_requestedData) {
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
    qCDebug(QT_REMOTEOBJECT_MODELS) << "Final requests" << finalRequests;
    Q_FOREACH (const RequestedData &data, finalRequests) {
        QRemoteObjectPendingReply<DataEntries> reply = replicaRowRequest(data.start, data.end, data.roles);
        RowWatcher *watcher = new RowWatcher(data.start, data.end, data.roles, reply);
        connect(watcher, &RowWatcher::finished, this, &QAbstractItemReplicaPrivate::requestedData);
    }
    m_requestedData.clear();
}

void QAbstractItemReplicaPrivate::onModelReset()
{
    q->beginResetModel();
    m_rows.clear();
    m_rows.resize(rowCount());
    q->endResetModel();
}

void QAbstractItemReplicaPrivate::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    // TODO clean cache
    const int index = orientation == Qt::Horizontal ? 0 : 1;
    for (int i = first; i < last; ++i )
        m_headerData[index][i].clear();
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
    typedef QPair<int, QVariant> Data;
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
        QVector<Data> &dat = m_headerData[index][watcher->sections[i]];
        bool found = false;
        for (int j = 0; j < dat.size(); ++j) {
            if (dat[j].first == watcher->roles[i]) {
                found = true;
                dat[j].second = data[i];
            }
        }
        if (!found)
            dat.append(qMakePair(watcher->roles[i], data[i]));

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
    connect(rep, &QAbstractItemReplicaPrivate::dataChanged, rep, &QAbstractItemReplicaPrivate::onDataChanged);
    connect(rep, &QAbstractItemReplicaPrivate::rowsInserted, rep, &QAbstractItemReplicaPrivate::onRowsInserted);
    connect(rep, &QAbstractItemReplicaPrivate::rowsRemoved, rep, &QAbstractItemReplicaPrivate::onRowsRemoved);
    connect(rep, &QAbstractItemReplicaPrivate::rowsMoved, rep, &QAbstractItemReplicaPrivate::onRowsMoved);
    connect(rep, &QAbstractItemReplicaPrivate::modelReset, d.data(), &QAbstractItemReplicaPrivate::onModelReset);
    connect(rep, &QAbstractItemReplicaPrivate::headerDataChanged, rep, &QAbstractItemReplicaPrivate::onHeaderDataChanged);
}

QAbstractItemReplica::~QAbstractItemReplica()
{
}

QModelIndex QAbstractItemReplica::parent(const QModelIndex & /*index*/) const
{
//    if (index.isValid())
//        return QModelIndex();
    return QModelIndex();
}
int QAbstractItemReplica::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;
    return d->columnCount();
}

QVariant findData(const CachedRowEntry &row, const QModelIndex &index, int role, bool *cached = 0)
{
    if (cached)
        *cached = true;
    if (index.column() >= row.size()) {
        if (cached)
            *cached = false;
        return QVariant();
    }
    const CacheEntry &entry = row[index.column()];
    CacheEntry::const_iterator res = std::find_if(entry.constBegin(), entry.constEnd(), CacheRoleComparator(role));
    if (res != entry.constEnd()) {
        return res->second;
    } else {
        if (cached)
            *cached = false;
        return QVariant();
    }
    if (cached)
        *cached = false;
    return QVariant();
}

int findCacheBorder(int start, int range, const CacheData &data) {
    const int limit = std::min(std::max(start + range, 0), data.size());
    const int step = limit < start ? -1 : 1;
    for (int i = start + step; i < limit; i += step) {
        if (!data.at(i).isEmpty() || i < 0 || i > data.size())
            return i - step;
    }
    return start;
}

QVariant QAbstractItemReplica::data(const QModelIndex & index, int role) const
{
    if (!(role == Qt::DisplayRole || role == Qt::BackgroundColorRole))
        return QVariant();
    const int row = index.row();
    const int col = index.column();
    if (col < 0 || col >= d->columnCount())
        return QVariant();
    if (row < 0 || row >= d->rowCount())
        return QVariant();
    if (!d->isInitialized())
        return QVariant();
    bool cached = false;
    QVariant result = findData(d->m_rows.at(index.row()), index, role, &cached);
    if (!cached) {
        const int low = findCacheBorder(row, -HalfLookAhead, d->m_rows);
        const int high = findCacheBorder(row, HalfLookAhead, d->m_rows);
        Q_ASSERT(low <= row);
        Q_ASSERT(high >= row);
        Q_ASSERT(low >= 0);
        Q_ASSERT(high <= d->m_rows.size());
        const int count = high - low;
        IndexList start, end;
        start << ModelIndex(low, 0);
        end << ModelIndex(low+count, d->columnCount() -1);
        RequestedData data;
        QVector<int> roles;
        roles << role;
        data.start = start;
        data.end = end;
        data.roles = roles;
        d->m_requestedData.push_back(data);
        QMetaObject::invokeMethod(d.data(), "fetchPendingData", Qt::QueuedConnection);
    }
    return result;
}
QModelIndex QAbstractItemReplica::index(int row, int column, const QModelIndex & /*parent*/) const
{
    return createIndex(row, column);
}
int QAbstractItemReplica::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;
    return d->rowCount();
}

QVariant QAbstractItemReplica::headerData(int section, Qt::Orientation orientation, int role) const
{
    typedef QPair<int,QVariant> Entry;

    const int index = orientation == Qt::Horizontal ? 0 : 1;
    const QVector<CacheEntry> elem = d->m_headerData[index];
    if (section >= elem.size())
        return QVariant();

    const QVector<Entry> entries = elem.at(section);
    Q_FOREACH (const Entry &entry, entries)
        if (entry.first == role)
            return entry.second;

    RequestedHeaderData data;
    data.role = role;
    data.section = section;
    data.orientation = orientation;
    d->m_requestedHeaderData.push_back(data);
    QMetaObject::invokeMethod(d.data(), "fetchPendingHeaderData", Qt::QueuedConnection);
    return QVariant();
}

bool QAbstractItemReplica::isInitialized() const
{
    return d->isInitialized();
}

QVector<int> QAbstractItemReplica::availableRoles() const
{
    return d->availableRoles();
}

QT_END_NAMESPACE
