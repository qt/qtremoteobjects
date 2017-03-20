/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QTreeView>
#include <QApplication>
#include <QRemoteObjectNode>
#include <QTimer>
#include <QStandardItemModel>
#include <QStandardItem>


struct TimerHandler : public QObject
{
    Q_OBJECT
public:
    QStandardItemModel *model;
public Q_SLOTS:
    void changeData() {
        Q_ASSERT(model);
        Q_ASSERT(model->rowCount() > 50);
        Q_ASSERT(model->columnCount() > 1);
        for (int i = 10; i < 50; ++i)
            model->setData(model->index(i, 1), QColor(Qt::blue), Qt::BackgroundRole);
    }
    void insertData() {
        Q_ASSERT(model);
        Q_ASSERT(model->rowCount() > 50);
        Q_ASSERT(model->columnCount() > 1);
        model->insertRows(2, 9);
        for (int i = 2; i < 11; ++i) {
            model->setData(model->index(i, 1), QColor(Qt::green), Qt::BackgroundRole);
            model->setData(model->index(i, 1), QLatin1String("InsertedRow"), Qt::DisplayRole);
        }
    }
    void removeData() {
        model->removeRows(2, 4);
    }

    void changeFlags() {
        QStandardItem *item = model->item(0, 0);
        item->setEnabled(false);
        item = item->child(0, 0);
        item->setFlags(item->flags() & Qt::ItemIsSelectable);
    }

    void moveData() {
        model->moveRows(QModelIndex(), 2, 4, QModelIndex(), 10);
    }
};

QList<QStandardItem*> addChild(int numChildren, int nestingLevel)
{
    QList<QStandardItem*> result;
    if (nestingLevel == 0)
        return result;
    for (int i = 0; i < numChildren; ++i) {
        QStandardItem *child = new QStandardItem(QStringLiteral("Child num %1, nesting Level %2").arg(i+1).arg(nestingLevel));
        if (i == 0)
            child->appendRow(addChild(numChildren, nestingLevel -1));
        result.push_back(child);
    }
    return result;
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules("qt.remoteobjects.debug=false\n"
                                     "qt.remoteobjects.warning=false");
    QApplication app(argc, argv);

    const int modelSize = 100000;
    QStringList list;
    QStandardItemModel sourceModel;
    QStringList hHeaderList;
    hHeaderList << QStringLiteral("First Column with spacing") << QStringLiteral("Second Column with spacing");
    sourceModel.setHorizontalHeaderLabels(hHeaderList);
    list.reserve(modelSize);
    for (int i = 0; i < modelSize; ++i) {
        QStandardItem *firstItem = new QStandardItem(QStringLiteral("FancyTextNumber %1").arg(i));
        if (i == 0)
            firstItem->appendRow(addChild(2, 2));
        QStandardItem *secondItem = new QStandardItem(QStringLiteral("FancyRow2TextNumber %1").arg(i));
        if (i % 2 == 0)
            firstItem->setBackground(Qt::red);
        QList<QStandardItem*> row;
        row << firstItem << secondItem;
        sourceModel.invisibleRootItem()->appendRow(row);
        //sourceModel.appendRow(row);
        list << QStringLiteral("FancyTextNumber %1").arg(i);
    }

    // Needed by QMLModelViewClient
    QHash<int,QByteArray> roleNames = {
        {Qt::DisplayRole, "_text"},
        {Qt::BackgroundRole, "_color"}
    };
    sourceModel.setItemRoleNames(roleNames);

    QVector<int> roles;
    roles << Qt::DisplayRole << Qt::BackgroundRole;

    qDebug() << "Creating registry host";
    QRemoteObjectRegistryHost node(QUrl(QStringLiteral("local:registry")));

    QRemoteObjectHost node2(QUrl(QStringLiteral("local:replica")), QUrl(QStringLiteral("local:registry")));
    node2.enableRemoting(&sourceModel, QStringLiteral("RemoteModel"), roles);

    QTreeView view;
    view.setWindowTitle(QStringLiteral("SourceView"));
    view.setModel(&sourceModel);
    view.show();
    TimerHandler handler;
    handler.model = &sourceModel;
    QTimer::singleShot(5000, &handler, SLOT(changeData()));
    QTimer::singleShot(10000, &handler, SLOT(insertData()));
    QTimer::singleShot(11000, &handler, SLOT(changeFlags()));
    QTimer::singleShot(12000, &handler, SLOT(removeData()));
    QTimer::singleShot(13000, &handler, SLOT(moveData()));

    return app.exec();
}

#include "main.moc"
