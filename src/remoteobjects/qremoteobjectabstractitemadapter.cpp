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

#include "qremoteobjectabstractitemadapter_p.h"

struct IndexRange
{
    IndexRange() {}
    IndexRange(const QModelIndex &parent, int startRow, int endRow, int startColumn, int endColumn)
        : parent(parent)
        , startRow(startRow)
        , endRow(endRow)
        , startColumn(startColumn)
        , endColumn(endColumn)
    {}

    QModelIndex parent;
    int startRow;
    int endRow;
    int startColumn;
    int endColumn;
};

// consider evaluating performance difference with item data
inline QVariantList collectData(const QModelIndex &index, const QAbstractItemModel *model, const QVector<int> &roles)
{
    QVariantList result;
    Q_FOREACH (int role, roles)
        result << model->data(index, role);
    return result;
}

inline QVector<int> filterRoles(const QVector<int> &roles, const QVector<int> &availableRoles)
{
    if (roles.isEmpty())
        return availableRoles;

    QVector<int> neededRoles;
    foreach (int inRole, roles) {
        foreach (int availableRole, availableRoles)
            if (inRole == availableRole) {
                neededRoles << inRole;
                continue;
            }
    }
    return neededRoles;
}

QAbstractItemSourceAdapter::QAbstractItemSourceAdapter(QAbstractItemModel *obj, const QVector<int> &roles)
    : QObject(obj),
      m_model(obj),
      m_availableRoles(roles)
{
    registerTypes();
    m_rowCount = m_model->rowCount(QModelIndex());
    m_columnCount = m_model->columnCount();
    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(sourceDataChanged(QModelIndex,QModelIndex,QVector<int>)));
    connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
    connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
    connect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(sourceRowsMoved(QModelIndex,int,int,QModelIndex,int)));
}

void QAbstractItemSourceAdapter::registerTypes()
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

// start and end need to have the same nesting depth and will fetch ALL children
DataEntries QAbstractItemSourceAdapter::replicaRowRequest(IndexList start, IndexList end, QVector<int> roles)
{
    DataEntries entries;
    if (start.isEmpty() || end.isEmpty()) {
        qCWarning(QT_REMOTEOBJECT_MODELS) << "requested indices are empty" << start << end << roles;
        return entries;
    }

    if (roles.isEmpty())
        roles << Qt::DisplayRole << Qt::BackgroundRole; //FIX
    Q_ASSERT(start.size() == end.size());

    qCWarning(QT_REMOTEOBJECT_MODELS) << "Requested rows" << start << end << roles;

    IndexList parentList = start;
    parentList.pop_back();
    QModelIndex parent = toQModelIndex(parentList, m_model);
    const ModelIndex currentIndex = start.last();
    const ModelIndex lastIndex = end.last();

    QVector<IndexRange> stack;
    stack.push_back(IndexRange(parent, currentIndex.row, lastIndex.row, currentIndex.column, lastIndex.column));
    while (!stack.isEmpty()) {
        IndexRange range = stack.back();
        QModelIndex parent = range.parent;

        const int rowCount = qMin(range.endRow, m_model->rowCount(parent) - 1);
        const int columnCount = qMin(range.endColumn, m_model->columnCount(parent) - 1);
        for (int row = range.startRow; row <= rowCount; ++row) {
            for (int column = range.startColumn; column <= columnCount; ++column) {
                const QModelIndex current = m_model->index(row, column, parent);
                entries << qMakePair(toModelIndexList(current, m_model), collectData(current, m_model, roles));

                const int rowCount = m_model->rowCount(current);
                const int columnCount = m_model->columnCount(current);
                if (rowCount && columnCount && current != parent)
                    stack.push_back(IndexRange(current, 0, rowCount, 0, columnCount));
            }
        }
        stack.pop_back();
    }
    return entries;
}

QVariantList QAbstractItemSourceAdapter::replicaHeaderRequest(QVector<Qt::Orientation> orientations, QVector<int> sections, QVector<int> roles)
{
    QVariantList data;
    Q_ASSERT(roles.size() == sections.size());
    Q_ASSERT(roles.size() == orientations.size());
    for (int i = 0; i < roles.size(); ++i) {
        data << m_model->headerData(sections[i], orientations[i], roles[i]);
    }
    return data;
}

void QAbstractItemSourceAdapter::sourceDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles) const
{
    QVector<int> neededRoles = filterRoles(roles, availableRoles());
    if (neededRoles.isEmpty())
        return;

    emit dataChanged(toModelIndexList(topLeft, m_model), toModelIndexList(bottomRight, m_model), neededRoles);
}

void QAbstractItemSourceAdapter::sourceRowsInserted(const QModelIndex & parent, int start, int end)
{
    setRowCount(m_model->rowCount());
    emit rowsInserted(toModelIndexList(parent, m_model), start, end);
}

void QAbstractItemSourceAdapter::sourceRowsRemoved(const QModelIndex & parent, int start, int end)
{
    setRowCount(m_model->rowCount());
    emit rowsRemoved(toModelIndexList(parent, m_model), start, end);
}

void QAbstractItemSourceAdapter::sourceRowsMoved(const QModelIndex & sourceParent, int sourceRow, int count, const QModelIndex & destinationParent, int destinationChild) const
{
    emit rowsMoved(toModelIndexList(sourceParent, m_model), sourceRow, count, toModelIndexList(destinationParent, m_model), destinationChild);
}
