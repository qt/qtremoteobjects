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

#include <QtTest/QtTest>
#include <QMetaType>
#include <QRemoteObjectReplica>
#include <QRemoteObjectNode>
#include <QAbstractItemReplica>
#include <QStandardItemModel>

namespace {

void compareData(const QAbstractItemModel *sourceModel, const QAbstractItemReplica *replica)
{
    for (int i = 0; i < sourceModel->rowCount(); ++i) {
        for (int j = 0; j < sourceModel->columnCount(); ++j) {
            foreach (int role, replica->availableRoles()) {
                QCOMPARE(replica->index(i, j).data(role), sourceModel->index(i, j).data(role));
            }
        }
    }
}

}

class TestModelView: public QObject
{
    Q_OBJECT

    QRemoteObjectNode m_basicServer;
    QRemoteObjectNode m_client;
    QRemoteObjectNode m_registryServer;
    QStandardItemModel m_sourceModel;

private slots:
    void initTestCase();

    void testEmptyModel();
    void testInitialData();
    void testHeaderData();
    void testDataChanged();
    void testDataInsertion();
    void testDataRemoval();

    void cleanup();
};

void TestModelView::initTestCase()
{
    QLoggingCategory::setFilterRules("*.debug=true\n"
                                     "qt.remoteobjects.debug=false\n"
                                     "qt.remoteobjects.warning=false");
    //Setup registry
    //Registry needs to be created first until we get the retry mechanism implemented
    m_registryServer = QRemoteObjectNode::createRegistryHostNode();

    m_basicServer = QRemoteObjectNode::createHostNodeConnectedToRegistry();

    static const int modelSize = 20;
    QVector<int> roles = QVector<int>() << Qt::DisplayRole << Qt::BackgroundRole;

    QStringList list;
    list.reserve(modelSize);

    QStringList hHeaderList;
    hHeaderList << QStringLiteral("First Column with spacing") << QStringLiteral("Second Column with spacing");
    m_sourceModel.setHorizontalHeaderLabels(hHeaderList);

    for (int i = 0; i < modelSize; ++i) {
        QStandardItem *firstItem = new QStandardItem(QStringLiteral("FancyTextNumber %1").arg(i));
        QStandardItem *secondItem = new QStandardItem(QStringLiteral("FancyRow2TextNumber %1").arg(i));
        if (i % 2 == 0)
            firstItem->setBackground(Qt::red);
        QList<QStandardItem*> row;
        row << firstItem << secondItem;
        m_sourceModel.appendRow(row);
        list << QStringLiteral("FancyTextNumber %1").arg(i);
    }
    m_basicServer.enableRemoting(&m_sourceModel, "test", roles);

    m_client = QRemoteObjectNode::createNodeConnectedToRegistry();
}

void TestModelView::testEmptyModel()
{
    QVector<int> roles = QVector<int>() << Qt::DisplayRole << Qt::BackgroundRole;
    QStandardItemModel emptyModel;
    m_basicServer.enableRemoting(&emptyModel, "emptyModel", roles);

    QScopedPointer<QAbstractItemReplica> model(m_client.acquireModel("emptyModel"));
    QSignalSpy spy(model.data(), SIGNAL(initialized()));
    spy.wait();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(model->rowCount(), emptyModel.rowCount());
    QCOMPARE(model->columnCount(), emptyModel.columnCount());

    compareData(&emptyModel, model.data());
}

void TestModelView::testInitialData()
{
    QScopedPointer<QAbstractItemReplica> model(m_client.acquireModel("test"));
    QSignalSpy spy(model.data(), SIGNAL(initialized()));
    spy.wait();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(model->rowCount(), m_sourceModel.rowCount());
    QCOMPARE(model->columnCount(), m_sourceModel.columnCount());

    compareData(&m_sourceModel, model.data());
}

void TestModelView::testHeaderData()
{
    QScopedPointer<QAbstractItemReplica> model(m_client.acquireModel("test"));
    QSignalSpy spy(model.data(), SIGNAL(initialized()));
    spy.wait();
    // ask for all Data members first, so we don't have to wait for update signals
    QSignalSpy spyHeader(model.data(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)));
    for (int i = 0; i < m_sourceModel.rowCount(); ++i)
        model->headerData(i, Qt::Vertical, Qt::DisplayRole);
    for (int i = 0; i < m_sourceModel.columnCount(); ++i)
        model->headerData(i, Qt::Horizontal, Qt::DisplayRole);
    spyHeader.wait();
    for (int i = 0; i < m_sourceModel.rowCount(); ++i)
        QCOMPARE(model->headerData(i, Qt::Vertical, Qt::DisplayRole), m_sourceModel.headerData(i, Qt::Vertical, Qt::DisplayRole));
    for (int i = 0; i < m_sourceModel.columnCount(); ++i)
        QCOMPARE(model->headerData(i, Qt::Horizontal, Qt::DisplayRole), m_sourceModel.headerData(i, Qt::Horizontal, Qt::DisplayRole));
}

void TestModelView::testDataChanged()
{
    QScopedPointer<QAbstractItemReplica> model(m_client.acquireModel("test"));
    QSignalSpy spy(model.data(), SIGNAL(initialized()));
    spy.wait();
    QSignalSpy dataChangedSpy(model.data(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)));
    for (int i = 10; i < 20; ++i)
        m_sourceModel.setData(m_sourceModel.index(i, 1), QColor(Qt::blue), Qt::BackgroundRole);

    bool signalsReceived = false;
    while (dataChangedSpy.wait()) {
        signalsReceived = true;
        if (dataChangedSpy.takeLast().at(1).value<QModelIndex>().row() == 19)
            break;
    }
    QVERIFY(signalsReceived);
    compareData(&m_sourceModel, model.data());
}

void TestModelView::testDataInsertion()
{
    QScopedPointer<QAbstractItemReplica> model(m_client.acquireModel("test"));
    QSignalSpy spy(model.data(), SIGNAL(initialized()));
    spy.wait();
    QSignalSpy rowSpy(model.data(), SIGNAL(rowsInserted(QModelIndex,int,int)));
    m_sourceModel.insertRows(2, 9);
    rowSpy.wait();
    QCOMPARE(rowSpy.count(), 1);
    QCOMPARE(m_sourceModel.rowCount(), model->rowCount());

    // change one row to check for inconsistencies
    QSignalSpy dataChangedSpy(model.data(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)));
    m_sourceModel.setData(m_sourceModel.index(0, 1), QColor(Qt::green), Qt::BackgroundRole);
    m_sourceModel.setData(m_sourceModel.index(0, 1), QLatin1String("foo"), Qt::DisplayRole);

    dataChangedSpy.wait();
    QVERIFY(dataChangedSpy.count() > 0);
    compareData(&m_sourceModel, model.data());
}

void TestModelView::testDataRemoval()
{
    m_sourceModel.removeRows(2, 4);
    //Maybe some checks here?
}

void TestModelView::cleanup()
{
    // wait for delivery of RemoveObject events to the source
    QTest::qWait(20);
}

QTEST_MAIN(TestModelView)

#include "tst_modelview.moc"
