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
****************************************************************************/

#ifndef QREMOTEOBJECTPENDINGCALL_P_H
#define QREMOTEOBJECTPENDINGCALL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qremoteobjectpendingcall.h"

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

class QRemoteObjectPendingCallWatcherHelper;
class QRemoteObjectReplicaImplementation;

class QRemoteObjectPendingCallData : public QSharedData
{
public:
    typedef QExplicitlySharedDataPointer<QRemoteObjectPendingCallData> Ptr;

    explicit QRemoteObjectPendingCallData(int serialId = -1, QRemoteObjectReplicaImplementation *replica = nullptr);
    ~QRemoteObjectPendingCallData();

    QRemoteObjectReplicaImplementation *replica;
    int serialId;

    QVariant returnValue;
    QRemoteObjectPendingCall::Error error;

    mutable QMutex mutex;

    mutable QScopedPointer<QRemoteObjectPendingCallWatcherHelper> watcherHelper;
};

class QRemoteObjectPendingCallWatcherHelper: public QObject
{
    Q_OBJECT
public:
    void add(QRemoteObjectPendingCallWatcher *watcher);

    void emitSignals();

Q_SIGNALS:
    void finished();
};

QT_END_NAMESPACE

#endif
