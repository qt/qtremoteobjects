/****************************************************************************
**
** Copyright (C) 2014-2016 Ford Motor Company
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

#ifndef QQNXNATIVEIO_P_H
#define QQNXNATIVEIO_P_H

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

#include "qconnection_qnx_qiodevices.h"

#include <QReadWriteLock>
#include <QScopedPointer>

#include "private/qiodevice_p.h"
#include "private/qringbuffer_p.h"

QT_BEGIN_NAMESPACE

class QQnxNativeIoPrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(QQnxNativeIo)

    mutable QReadWriteLock ibLock;
    mutable QReadWriteLock obLock;

public:
    QQnxNativeIoPrivate();
    ~QQnxNativeIoPrivate();
    void thread_func();
    bool establishConnection();
    void teardownConnection();
    void stopThread();
    QString serverName;
    int serverId, channelId, connectionId;
    QAbstractSocket::SocketState state;
    QScopedPointer<QRingBuffer> obuffer;
    MsgType msgType;
    iov_t tx_iov[3], rx_iov[2];
    Thread<QQnxNativeIoPrivate> thread;
};

class QIOQnxSourcePrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(QIOQnxSource)

    friend class QQnxNativeServerPrivate;

    mutable QReadWriteLock ibLock;
    mutable QReadWriteLock obLock;

public:
    QIOQnxSourcePrivate(int _rcvid);
    int rcvid;
    QAbstractSocket::SocketState state;
    MsgType msgType;
    sigevent m_event;
    QRingBuffer obuffer;
    QAtomicInt m_serverClosing;
};

QT_END_NAMESPACE

#endif // QQNXNATIVEIO_P_H

