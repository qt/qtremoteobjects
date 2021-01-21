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

#ifndef QREMOTEOBJECTPENDINGCALL_H
#define QREMOTEOBJECTPENDINGCALL_H

#include <QtRemoteObjects/qtremoteobjectglobal.h>

#include <QtCore/qvariant.h>

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
    friend class QConnectedReplicaImplementation;
};

QT_END_NAMESPACE
Q_DECLARE_METATYPE(QRemoteObjectPendingCall)
QT_BEGIN_NAMESPACE

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectPendingCallWatcher: public QObject, public QRemoteObjectPendingCall
{
    Q_OBJECT

public:
    QRemoteObjectPendingCallWatcher(const QRemoteObjectPendingCall &call, QObject *parent = nullptr);
    ~QRemoteObjectPendingCallWatcher() override;

    bool isFinished() const;

    void waitForFinished();

Q_SIGNALS:
    void finished(QRemoteObjectPendingCallWatcher *self);

private:
    Q_DECLARE_PRIVATE(QRemoteObjectPendingCallWatcher)
};

template<typename T>
class QRemoteObjectPendingReply : public QRemoteObjectPendingCall
{
public:
    typedef T Type;

    QRemoteObjectPendingReply() = default;
    explicit QRemoteObjectPendingReply(const QRemoteObjectPendingCall &call)
        : QRemoteObjectPendingCall(call)
    {
    }

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

// NOTE: manual expansion of Q_DECLARE_METATYPE_TEMPLATE_1ARG, minus the IsSequentialContainer
template <typename T>
struct QMetaTypeId< QRemoteObjectPendingReply<T> >
{
    enum {
        Defined = QMetaTypeId2<T>::Defined
    };
    static int qt_metatype_id()
    {
        static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0);
        if (const int id = metatype_id.loadRelaxed())
            return id;
        const char *tName = QMetaType::typeName(qMetaTypeId<T>());
        Q_ASSERT(tName);
        const int tNameLen = int(qstrlen(tName));
        QByteArray typeName;
        typeName.reserve(int(sizeof("QRemoteObjectPendingReply")) + 1 + tNameLen + 1 + 1);
        typeName.append("QRemoteObjectPendingReply", int(sizeof("QRemoteObjectPendingReply")) - 1)
            .append('<').append(tName, tNameLen);
        if (typeName.endsWith('>'))
            typeName.append(' ');
        typeName.append('>');
        const int newId = qRegisterNormalizedMetaType< QRemoteObjectPendingReply<T> >(
                        typeName,
                        reinterpret_cast< QRemoteObjectPendingReply<T> *>(quintptr(-1)));
        metatype_id.storeRelease(newId);
        return newId;
    }
};

QT_END_NAMESPACE

#endif
