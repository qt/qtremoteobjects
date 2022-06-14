// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCoreApplication>
#include <QTimer>
#include "rep_timemodel_replica.h"

class tester : public QObject
{
    Q_OBJECT
public:
    tester() : QObject(nullptr)
    {
        QRemoteObjectNode m_client(QUrl(QStringLiteral("local:registry")));
        ptr1.reset(m_client.acquire< MinuteTimerReplica >());
        ptr2.reset(m_client.acquire< MinuteTimerReplica >());
        ptr3.reset(m_client.acquire< MinuteTimerReplica >());
        QTimer::singleShot(0, this, &tester::clear);
        QTimer::singleShot(1, this, &tester::clear);
        QTimer::singleShot(10000, this, &tester::clear);
        QTimer::singleShot(11000, this, &tester::clear);
    }
public slots:
    void clear()
    {
        static int i = 0;
        if (i == 0) {
            i++;
            ptr1.reset();
        } else if (i == 1) {
            i++;
            ptr2.reset();
        } else if (i == 2) {
            i++;
            ptr3.reset();
        } else {
            qApp->quit();
        }
    }

private:
    QScopedPointer<MinuteTimerReplica> ptr1, ptr2, ptr3;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    tester t;
    return a.exec();
}

#include "main.moc"
