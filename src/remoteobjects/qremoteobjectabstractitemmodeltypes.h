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

#ifndef QREMOTEOBJECTS_ABSTRACT_ITEM_MODEL_TYPES_H
#define QREMOTEOBJECTS_ABSTRACT_ITEM_MODEL_TYPES_H

#include <QList>
#include <QVector>
#include <QPair>
#include <QVariant>
#include <QModelIndex>
#include <QItemSelectionModel>
#include <QDebug>
#include <qnamespace.h>
#include <QtRemoteObjects/qtremoteobjectglobal.h>

QT_BEGIN_NAMESPACE

struct ModelIndex
{
    ModelIndex() : row(-1), column(-1) {}
    ModelIndex(int row_, int column_)
        : row(row_)
        , column(column_)
    {}

    inline bool operator==(const ModelIndex &other) const { return row == other.row && column == other.column; }
    inline bool operator!=(const ModelIndex &other) const { return !(*this == other); }
    int row;
    int column;
};

typedef QList<ModelIndex> IndexList;

struct IndexValuePair
{
    IndexValuePair(const IndexList index_ = IndexList(), const QVariantList &data_ = QVariantList(), int rowCount_ = -1, int columnCount_ = -1)
        : index(index_)
        , data(data_)
        , rowCount(rowCount_)
        , columnCount(columnCount_)
    {}

    inline bool operator==(const IndexValuePair &other) const { return rowCount == other.rowCount && columnCount == other.columnCount && index == other.index && data == other.data; }
    inline bool operator!=(const IndexValuePair &other) const { return !(*this == other); }

    IndexList index;
    QVariantList data;
    int rowCount;
    int columnCount;
};

struct DataEntries
{
    DataEntries(int rowCount_ = -1, int columnCount_ = -1)
        : rowCount(rowCount_)
        , columnCount(columnCount_)
    {}

    inline bool operator==(const DataEntries &other) const { return rowCount == other.rowCount && columnCount == other.columnCount && data == other.data; }
    inline bool operator!=(const DataEntries &other) const { return !(*this == other); }

    QVector<IndexValuePair> data;
    int rowCount;
    int columnCount;
};

inline QDebug& operator<<(QDebug &stream, const ModelIndex &index)
{
    return stream.nospace() << "ModelIndex[row=" << index.row << ", column=" << index.column << "]";
}

inline QDataStream& operator<<(QDataStream &stream, const ModelIndex &index)
{
    return stream << index.row << index.column;
}

inline QDataStream& operator>>(QDataStream &stream, ModelIndex &index)
{
    return stream >> index.row >> index.column;
}

inline QDataStream& operator<<(QDataStream &stream, Qt::Orientation orient)
{
    return stream << static_cast<int>(orient);
}

inline QDataStream& operator>>(QDataStream &stream, Qt::Orientation &orient)
{
    int val;
    QDataStream &ret = stream >> val;
    orient = static_cast<Qt::Orientation>(val);
    return ret;
}

inline QDataStream& operator<<(QDataStream &stream, QItemSelectionModel::SelectionFlags command)
{
    return stream << static_cast<int>(command);
}

inline QDataStream& operator>>(QDataStream &stream, QItemSelectionModel::SelectionFlags &command)
{
    int val;
    QDataStream &ret = stream >> val;
    command = static_cast<QItemSelectionModel::SelectionFlags>(val);
    return ret;
}

inline QDebug& operator<<(QDebug &stream, const DataEntries &entries)
{
    return stream.nospace() << "DataEntries[rowCount=" << entries.rowCount << ", columnCount=" << entries.columnCount << ", data=" << entries.data << "]";
}

inline QDataStream& operator<<(QDataStream &stream, const DataEntries &entries)
{
    return stream << entries.rowCount << entries.columnCount << entries.data;
}

inline QDataStream& operator>>(QDataStream &stream, DataEntries &entries)
{
    return stream >> entries.rowCount >> entries.columnCount >> entries.data;
}

inline QDebug& operator<<(QDebug &stream, const IndexValuePair &pair)
{
    return stream.nospace() << "IndexValuePair[index=" << pair.index << ", data=" << pair.data << "]";
}

inline QDataStream& operator<<(QDataStream &stream, const IndexValuePair &pair)
{
    return stream << pair.index << pair.data << pair.rowCount << pair.columnCount;
}

inline QDataStream& operator>>(QDataStream &stream, IndexValuePair &pair)
{
    return stream >> pair.index >> pair.data >> pair.rowCount >> pair.columnCount;
}

inline QModelIndex toQModelIndex(const IndexList &list, const QAbstractItemModel *model)
{
    QModelIndex result;
    Q_FOREACH (const ModelIndex &index, list) {
        result = model->index(index.row, index.column, result);
        Q_ASSERT(result.isValid());
    }
    return result;
}

inline IndexList toModelIndexList(const QModelIndex &index, const QAbstractItemModel *model)
{
    IndexList list;
    if (index.isValid()) {
        list << ModelIndex(index.row(), index.column());
        for (QModelIndex curIndex = model->parent(index); curIndex.isValid(); curIndex = model->parent(curIndex))
            list.prepend(ModelIndex(curIndex.row(), curIndex.column()));
    }
    return list;
}

Q_DECLARE_METATYPE(ModelIndex)
Q_DECLARE_METATYPE(IndexList)
Q_DECLARE_METATYPE(DataEntries)
Q_DECLARE_METATYPE(IndexValuePair)
Q_DECLARE_METATYPE(Qt::Orientation)
Q_DECLARE_METATYPE(QItemSelectionModel::SelectionFlags)

QT_END_NAMESPACE

#endif // QREMOTEOBJECTS_ABSTRACT_ITEM_MODEL_TYPES_H
