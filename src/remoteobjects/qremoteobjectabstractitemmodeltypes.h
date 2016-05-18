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

#include <QDataStream>
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
    explicit IndexValuePair(const IndexList index_ = IndexList(), const QVariantList &data_ = QVariantList(), bool hasChildren_ = false, const Qt::ItemFlags &flags_ = Qt::ItemFlags())
        : index(index_)
        , data(data_)
        , flags(flags_)
        , hasChildren(hasChildren_)
    {}

    inline bool operator==(const IndexValuePair &other) const { return index == other.index && data == other.data && hasChildren == other.hasChildren && flags == other.flags; }
    inline bool operator!=(const IndexValuePair &other) const { return !(*this == other); }

    IndexList index;
    QVariantList data;
    Qt::ItemFlags flags;
    bool hasChildren;
};

struct DataEntries
{
    inline bool operator==(const DataEntries &other) const { return data == other.data; }
    inline bool operator!=(const DataEntries &other) const { return !(*this == other); }

    QVector<IndexValuePair> data;
};

inline QDebug operator<<(QDebug stream, const ModelIndex &index)
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

inline QDebug operator<<(QDebug stream, const DataEntries &entries)
{
    return stream.nospace() << "DataEntries[" << entries.data << "]";
}

inline QDataStream& operator<<(QDataStream &stream, const DataEntries &entries)
{
    return stream << entries.data;
}

inline QDataStream& operator>>(QDataStream &stream, DataEntries &entries)
{
    return stream >> entries.data;
}

inline QDebug operator<<(QDebug stream, const IndexValuePair &pair)
{
    return stream.nospace() << "IndexValuePair[index=" << pair.index << ", data=" << pair.data << ", hasChildren=" << pair.hasChildren << ", flags=" << pair.flags << "]";
}

inline QDataStream& operator<<(QDataStream &stream, const IndexValuePair &pair)
{
    return stream << pair.index << pair.data << pair.hasChildren << static_cast<int>(pair.flags);
}

inline QDataStream& operator>>(QDataStream &stream, IndexValuePair &pair)
{
    int flags;
    QDataStream &ret = stream >> pair.index >> pair.data >> pair.hasChildren >> flags;
    pair.flags = static_cast<Qt::ItemFlags>(flags);
    return ret;
}

inline QString modelIndexToString(const IndexList &list)
{
    QString s;
    QDebug(&s) << list;
    return s;
}

inline QString modelIndexToString(const ModelIndex &index)
{
    QString s;
    QDebug(&s) << index;
    return s;
}

inline QModelIndex toQModelIndex(const IndexList &list, const QAbstractItemModel *model, bool *ok = nullptr, bool ensureItem = false)
{
    QModelIndex result;
    for (int i = 0; i < list.count(); ++i) {
        const ModelIndex &index = list[i];
        if (ensureItem)
            const_cast<QAbstractItemModel *>(model)->setData(result, index.row, Qt::UserRole - 1);

        result = model->index(index.row, index.column, result);
        if (!result.isValid()) {
            if (ok) {
                *ok = false;
            } else {
                qFatal("Internal error: invalid index=%s in indexList=%s",
                       qPrintable(modelIndexToString(list[i])), qPrintable(modelIndexToString(list)));
            }
            return QModelIndex();
        }
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
