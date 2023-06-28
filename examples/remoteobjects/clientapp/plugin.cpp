// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "plugin.h"

// Implements a "TimeModel" class with hour and minute properties
// that change on-the-minute yet efficiently sleep the rest
// of the time.

TimeModel::TimeModel(QObject *parent) : QObject(parent), m_replica(nullptr)
{
    m_client.setRegistryUrl(QUrl(QStringLiteral("local:registry")));
    m_replica.reset(m_client.acquire<MinuteTimerReplica>());
    connect(m_replica.get(), &MinuteTimerReplica::hourChanged, this, &TimeModel::timeChanged);
    connect(m_replica.get(), &MinuteTimerReplica::minuteChanged, this, &TimeModel::timeChanged);
    connect(m_replica.get(), &MinuteTimerReplica::timeChanged, this, &TimeModel::timeChanged);
    connect(m_replica.get(), &MinuteTimerReplica::stateChanged, this, &TimeModel::isValidChanged);
}

TimeModel::~TimeModel() { }

int TimeModel::minute() const
{
    return m_replica->minute();
}

int TimeModel::hour() const
{
    return m_replica->hour();
}

bool TimeModel::isValid() const
{
    return m_replica->state() == QRemoteObjectReplica::Valid;
}
