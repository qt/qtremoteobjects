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

#include <QtQml/QQmlExtensionPlugin>
#include <QtQml/QQmlEngine>
#include <QtQml/qqml.h>
#include <QDebug>
#include <QDateTime>
#include <QBasicTimer>
#include <QCoreApplication>
#include <QRemoteObjectReplica>
#include <QRemoteObjectNode>
#include "rep_timemodel_replica.h"

// Implements a "TimeModel" class with hour and minute properties
// that change on-the-minute yet efficiently sleep the rest
// of the time.

QRemoteObjectNode m_client;

class TimeModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int hour READ hour NOTIFY timeChanged)
    Q_PROPERTY(int minute READ minute NOTIFY timeChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged)

public:
    TimeModel(QObject *parent=0) : QObject(parent), d_ptr(Q_NULLPTR)
    {
        d_ptr.reset(m_client.acquire< MinuteTimerReplica >());
        connect(d_ptr.data(), SIGNAL(hourChanged()), this, SIGNAL(timeChanged()));
        connect(d_ptr.data(), SIGNAL(minuteChanged()), this, SIGNAL(timeChanged()));
        connect(d_ptr.data(), SIGNAL(timeChanged()), this, SIGNAL(timeChanged()));
        connect(d_ptr.data(), SIGNAL(timeChanged2(QTime)), this, SLOT(test(QTime)));
        connect(d_ptr.data(), SIGNAL(sendCustom(PresetInfo)), this, SLOT(testCustom(PresetInfo)));
    }

    ~TimeModel()
    {
    }

    int minute() const { return d_ptr->minute(); }
    int hour() const { return d_ptr->hour(); }
    bool isValid() const { return d_ptr->state() == QRemoteObjectReplica::Valid; }

public slots:
    //Test a signal with parameters
    void test(QTime t)
    {
        qDebug()<<"Test"<<t;
        d_ptr->SetTimeZone(t.minute());
    }
    //Test a signal with a custom type
    void testCustom(PresetInfo info)
    {
        qDebug()<<"testCustom"<<info.presetNumber()<<info.frequency()<<info.stationName();
    }

signals:
    void timeChanged();
    void timeChanged2(QTime t);
    void sendCustom(PresetInfo info);
    void isValidChanged();

private:
    QScopedPointer<MinuteTimerReplica> d_ptr;
};

class QExampleQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(engine);
        Q_UNUSED(uri);
        Q_ASSERT(uri == QLatin1String("TimeExample"));
        engine->addImportPath(QStringLiteral("qrc:/qml"));
        m_client.setRegistryUrl(QUrl(QStringLiteral("local:registry")));
    }
    void registerTypes(const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("TimeExample"));
        qmlRegisterType<TimeModel>(uri, 1, 0, "Time");
    }

};

#include "plugin.moc"
