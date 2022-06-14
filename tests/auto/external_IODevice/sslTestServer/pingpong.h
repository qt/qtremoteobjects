// Copyright (C) 2018 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PINGPONG_H
#define PINGPONG_H

#include "rep_pingpong_source.h"

class PingPong : public PingPongSimpleSource
{
public:
    PingPong(QObject *parent = nullptr);

    // PingPongSource interface
public slots:
    void ping(const QString &message) override;
    void quit() override;
};

#endif // PINGPONG_H
