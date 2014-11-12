/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
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

#ifndef QREMOTEOBJECTPENDINGCALL_H
#define QREMOTEOBJECTPENDINGCALL_H

#include <QtRemoteObjects/qtremoteobjectglobal.h>

#include <QDebug>
#include <QVariant>

QT_BEGIN_NAMESPACE

class QRemoteObjectPendingCallWatcherPrivate;
class QRemoteObjectPendingCallData;

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectPendingCall
{
public:
    enum Error {
        NoError,
        InvalidMessage
    };

    QRemoteObjectPendingCall();
    QRemoteObjectPendingCall(const QRemoteObjectPendingCall &other);
    ~QRemoteObjectPendingCall();

    QRemoteObjectPendingCall &operator=(const QRemoteObjectPendingCall &other);

    QVariant returnValue() const;
    QRemoteObjectPendingCall::Error error() const;

    bool isFinished() const;

    bool waitForFinished(int timeout = 30000);

    static QRemoteObjectPendingCall fromCompletedCall(const QVariant &returnValue);

protected:
    QRemoteObjectPendingCall(QRemoteObjectPendingCallData *dd);

    /// Shared data, note: might be null
    QExplicitlySharedDataPointer<QRemoteObjectPendingCallData> d;

private:
    friend class QConnectedReplicaPrivate;
};

Q_DECLARE_METATYPE(QRemoteObjectPendingCall);

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectPendingCallWatcher: public QObject, public QRemoteObjectPendingCall
{
    Q_OBJECT

public:
    QRemoteObjectPendingCallWatcher(const QRemoteObjectPendingCall &call, QObject *parent = 0);
    ~QRemoteObjectPendingCallWatcher();

    bool isFinished() const;

    void waitForFinished();

Q_SIGNALS:
    void finished(QRemoteObjectPendingCallWatcher *self);

private:
    Q_DECLARE_PRIVATE(QRemoteObjectPendingCallWatcher)
    Q_PRIVATE_SLOT(d_func(), void _q_finished())
};

template<typename T>
class QRemoteObjectPendingReply : public QRemoteObjectPendingCall
{
public:
    typedef T Type;

    inline QRemoteObjectPendingReply()
    {
    }
    inline QRemoteObjectPendingReply(const QRemoteObjectPendingReply &other)
        : QRemoteObjectPendingCall(other)
    {
    }
    explicit inline QRemoteObjectPendingReply(const QRemoteObjectPendingCall &call)
    {
        *this = call;
    }
    inline ~QRemoteObjectPendingReply() {}

    QRemoteObjectPendingReply &operator=(const QRemoteObjectPendingCall &other)
    {
        QRemoteObjectPendingCall::operator=(other);
        return *this;
    }

    Type returnValue() const
    {
        return qvariant_cast<Type>(QRemoteObjectPendingCall::returnValue());
    }

};

QT_END_NAMESPACE

#endif
