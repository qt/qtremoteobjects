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

#ifndef TESTS_ENGINE_H
#define TESTS_ENGINE_H

#include "rep_engine_source.h"

class Engine : public EngineSimpleSource
{
    Q_OBJECT
    Q_PROPERTY(bool purchasedPart READ purchasedPart WRITE setpurchasedPart)

public:
    Engine(QObject *parent = nullptr);
    virtual ~Engine();

    bool start() Q_DECL_OVERRIDE;
    void increaseRpm(int deltaRpm) Q_DECL_OVERRIDE;

    void unnormalizedSignature(int, int) Q_DECL_OVERRIDE {}

    Temperature temperature() Q_DECL_OVERRIDE;
    void setTemperature(const Temperature &value);

    void setSharedTemperature(const Temperature::Ptr &) Q_DECL_OVERRIDE {}

    bool purchasedPart() {return _purchasedPart;}

public Q_SLOTS:
    void setpurchasedPart(bool value);

    QString myTestString() Q_DECL_OVERRIDE { return _myTestString; }
    void setMyTestString(QString value) Q_DECL_OVERRIDE { _myTestString = value; }

private:
    bool _purchasedPart;
    QString _myTestString;
    Temperature _temperature;
};

#endif
