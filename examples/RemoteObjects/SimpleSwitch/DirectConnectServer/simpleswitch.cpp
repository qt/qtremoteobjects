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
    if (currState()) // check if current state is true, currState() is defined in repc generated rep_SimpleSwitch_source.h
        setCurrState(false); // set state to false
    else
        setCurrState(true); // set state to true
    qDebug() << "Source State is "<<currState();

}

