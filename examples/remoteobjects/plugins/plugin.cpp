// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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

static QRemoteObjectNode m_client;

class TimeModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int hour READ hour NOTIFY timeChanged)
    Q_PROPERTY(int minute READ minute NOTIFY timeChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged)

public:
    TimeModel(QObject *parent = nullptr) : QObject(parent), d_ptr(nullptr)
    {
        d_ptr.reset(m_client.acquire< MinuteTimerReplica >());
        connect(d_ptr.data(), &MinuteTimerReplica::hourChanged, this, &TimeModel::timeChanged);
        connect(d_ptr.data(), &MinuteTimerReplica::minuteChanged, this, &TimeModel::timeChanged);
        connect(d_ptr.data(), &MinuteTimerReplica::timeChanged, this, &TimeModel::timeChanged);
        connect(d_ptr.data(), &MinuteTimerReplica::timeChanged2, this, &TimeModel::test);
        connect(d_ptr.data(), &MinuteTimerReplica::sendCustom, this, &TimeModel::testCustom);
    }

    ~TimeModel() override
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
    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(engine)
        Q_UNUSED(uri)
        Q_ASSERT(uri == QLatin1String("TimeExample"));
        engine->addImportPath(QStringLiteral("qrc:/qml"));
        m_client.setRegistryUrl(QUrl(QStringLiteral("local:registry")));
    }
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(uri == QLatin1String("TimeExample"));
        qmlRegisterType<TimeModel>(uri, 1, 0, "Time");
    }

};

#include "plugin.moc"
