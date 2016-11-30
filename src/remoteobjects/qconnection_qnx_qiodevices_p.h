/****************************************************************************
**
** Copyright (C) 2014-2016 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
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

#endif // QQNXNATIVEIO_P_H

