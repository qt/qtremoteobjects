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

// A "TimeModel" class with hour and minute properties
// that change on-the-minute yet efficiently sleep the rest
// of the time.

class TimeModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int hour READ hour NOTIFY timeChanged)
    Q_PROPERTY(int minute READ minute NOTIFY timeChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged)
    QML_NAMED_ELEMENT(Time)

public:
    TimeModel(QObject *parent = nullptr);
    ~TimeModel() override;

    int minute() const;
    int hour() const;
    bool isValid() const;

signals:
    void timeChanged();
    void isValidChanged();

private:
    QRemoteObjectNode m_client;
    std::unique_ptr<MinuteTimerReplica> m_replica;
};
