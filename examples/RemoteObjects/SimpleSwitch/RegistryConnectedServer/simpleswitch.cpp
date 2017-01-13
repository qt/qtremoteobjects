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

#include "simpleswitch.h"

// constructor
SimpleSwitch::SimpleSwitch(QObject *parent) : SimpleSwitchSimpleSource(parent)
{
    stateChangeTimer = new QTimer(this); // Initialize timer
    QObject::connect(stateChangeTimer, SIGNAL(timeout()), this, SLOT(timeout_slot())); // connect timeout() signal from stateChangeTimer to timeout_slot() of simpleSwitch
    stateChangeTimer->start(2000); // Start timer and set timout to 2 seconds
    qDebug() << "Source Node Started";
}

//destructor
SimpleSwitch::~SimpleSwitch()
{
    stateChangeTimer->stop();
}

void SimpleSwitch::server_slot(bool clientState)
{
    qDebug() << "Replica state is " << clientState; // print switch state echoed back by client
}

void SimpleSwitch::timeout_slot(void)
{
    // slot called on timer timeout
    if (currState()) // check if current state is true, currState() is defined in repc generated rep_simpleswitch_source.h
        setCurrState(false); // set state to false
    else
        setCurrState(true); // set state to true
    qDebug() << "Source State is "<<currState();

}

