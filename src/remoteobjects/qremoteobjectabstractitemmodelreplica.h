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

#ifndef QREMOTEOBJECTS_ABSTRACTITEMMODELREPLICA_H
#define QREMOTEOBJECTS_ABSTRACTITEMMODELREPLICA_H

#include <QtRemoteObjects/qtremoteobjectglobal.h>

#include <QAbstractItemModel>
#include <QItemSelectionModel>

QT_BEGIN_NAMESPACE

class QAbstractItemModelReplicaPrivate;

class Q_REMOTEOBJECTS_EXPORT QAbstractItemModelReplica : public QAbstractItemModel
{
    Q_OBJECT
public:
    ~QAbstractItemModelReplica();

    QItemSelectionModel* selectionModel() const;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex & index) const  Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QVector<int> availableRoles() const;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    bool isInitialized() const;
    bool hasData(const QModelIndex &index, int role) const;

Q_SIGNALS:
    void initialized();

private:
    explicit QAbstractItemModelReplica(QAbstractItemModelReplicaPrivate *rep);
    QScopedPointer<QAbstractItemModelReplicaPrivate> d;
    friend class QAbstractItemModelReplicaPrivate;
    friend class QRemoteObjectNode;
};

QT_END_NAMESPACE

#endif // QREMOTEOBJECTS_ABSTRACTITEMMODELREPLICA_H
