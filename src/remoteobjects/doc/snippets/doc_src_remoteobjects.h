/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

//! [qtremoteobject_include]
#include <QtRemoteObjects>
//! [qtremoteobject_include]

//! [implementing_source]
#include "rep_TimeModel_source.h"

class MinuteTimer : public MinuteTimerSource
{
Q_OBJECT
public:
    MinuteTimer(QObject *parent = nullptr);
    virtual ~MinuteTimer();

public slots:
    virtual void SetTimeZone(int zn) {  //this is a pure virtual method in MinuteTimerSource
        qDebug()<<"SetTimeZone"<<zn;
        if (zn != zone) {
            zone = zn;
        }
    };

protected:
    void timerEvent(QTimerEvent *);

private:
    QTime time;
    QBasicTimer timer;
    int zone;
};
//! [implementing_source]
