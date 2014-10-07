/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
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

#include <QCoreApplication>
#include <QTimer>
#include "rep_TimeModel_replica.h"

class tester : public QObject
{
    Q_OBJECT
public:
    tester() : QObject(Q_NULLPTR)
    {
        QRemoteObjectNode m_client = QRemoteObjectNode::createNodeConnectedToRegistry();;
        ptr1.reset(m_client.acquire< MinuteTimerReplica >());
        ptr2.reset(m_client.acquire< MinuteTimerReplica >());
        ptr3.reset(m_client.acquire< MinuteTimerReplica >());
        QTimer::singleShot(0,this,SLOT(clear()));
        QTimer::singleShot(1,this,SLOT(clear()));
        QTimer::singleShot(10000,this,SLOT(clear()));
        QTimer::singleShot(11000,this,SLOT(clear()));
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
