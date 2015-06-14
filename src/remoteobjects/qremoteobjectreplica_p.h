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

#ifndef QREMOTEOBJECTREPLICA_P_H
#define QREMOTEOBJECTREPLICA_P_H

#include "qremoteobjectreplica.h"

#include "qremoteobjectpendingcall.h"

#include <QPointer>
#include <QVector>
#include <qcompilerdetection.h>

QT_BEGIN_NAMESPACE

class QRemoteObjectReplica;
class QRemoteObjectSource;
class ClientIoDevice;

namespace QRemoteObjectPackets {
class QInitDynamicPacket;
class QInvokePacket;
class QInvokeReplyPacket;
class QRemoteObjectPacket;
}

class QRemoteObjectReplicaPrivate : public QObject
{
public:
    explicit QRemoteObjectReplicaPrivate(const QString &name, const QMetaObject *);
    virtual ~QRemoteObjectReplicaPrivate();

    bool isDynamicReplica() const;

    virtual const QVariant getProperty(int i) const = 0;
    virtual void setProperties(const QVariantList &) = 0;
    virtual void setProperty(int i, const QVariant &) = 0;
    virtual bool isShortCircuit() const = 0;
    virtual bool isInitialized() const { return true; }
    virtual bool isReplicaValid() const { return true; }
    virtual bool waitForSource(int) { return true; }
    virtual bool waitForFinished(const QRemoteObjectPendingCall &, int) { return true; }
    virtual void notifyAboutReply(const QRemoteObjectPackets::QInvokeReplyPacket *) {}
    virtual void configurePrivate(QRemoteObjectReplica *);
    void emitValidChanged();
    void emitInitialized();

    virtual void _q_send(QMetaObject::Call call, int index, const QVariantList &args) = 0;
    virtual QRemoteObjectPendingCall _q_sendWithReply(QMetaObject::Call call, int index, const QVariantList &args) = 0;

    //Dynamic replica functions
    virtual void initializeMetaObject(const QRemoteObjectPackets::QInitDynamicPacket *packet);

    QString m_objectName;
    const QMetaObject *m_metaObject;

    //Dynamic Replica data
    int m_numSignals;//TODO maybe here too
    int m_methodOffset;
    int m_signalOffset;
    int m_propertyOffset;
};

class QConnectedReplicaPrivate : public QRemoteObjectReplicaPrivate
{
public:
    explicit QConnectedReplicaPrivate(const QString &name, const QMetaObject *);
    virtual ~QConnectedReplicaPrivate();
    const QVariant getProperty(int i) const Q_DECL_OVERRIDE;
    void setProperties(const QVariantList &) Q_DECL_OVERRIDE;
    void setProperty(int i, const QVariant &) Q_DECL_OVERRIDE;
    bool isShortCircuit() const Q_DECL_OVERRIDE { return false; }
    bool isInitialized() const Q_DECL_OVERRIDE;
    bool isReplicaValid() const Q_DECL_OVERRIDE;
    bool waitForSource(int timeout) Q_DECL_OVERRIDE;
    void initialize(const QByteArray &);
    void configurePrivate(QRemoteObjectReplica *) Q_DECL_OVERRIDE;
    void requestRemoteObjectSource();
    bool sendCommand(const QRemoteObjectPackets::QRemoteObjectPacket *packet);
    QRemoteObjectPendingCall sendCommandWithReply(QRemoteObjectPackets::QInvokePacket* packet);
    bool waitForFinished(const QRemoteObjectPendingCall &call, int timeout);
    void notifyAboutReply(const QRemoteObjectPackets::QInvokeReplyPacket* replyPacket);
    void setConnection(ClientIoDevice *conn);
    void setDisconnected();

    void _q_send(QMetaObject::Call call, int index, const QVariantList &args) Q_DECL_OVERRIDE;
    QRemoteObjectPendingCall _q_sendWithReply(QMetaObject::Call call, int index, const QVariantList& args) Q_DECL_OVERRIDE;

    void initializeMetaObject(const QRemoteObjectPackets::QInitDynamicPacket *packet) Q_DECL_OVERRIDE;
    QAtomicInt isSet;
    QVector<QRemoteObjectReplica *> m_parentsNeedingConnect;
    QVariantList m_propertyStorage;
    QPointer<ClientIoDevice> connectionToSource;

    // pending call data
    int m_curSerialId;
    QHash<int, QRemoteObjectPendingCall> m_pendingCalls;
};

class QInProcessReplicaPrivate : public QRemoteObjectReplicaPrivate
{
public:
    explicit QInProcessReplicaPrivate(const QString &name, const QMetaObject *);
    virtual ~QInProcessReplicaPrivate();

    const QVariant getProperty(int i) const Q_DECL_OVERRIDE;
    void setProperties(const QVariantList &) Q_DECL_OVERRIDE;
    void setProperty(int i, const QVariant &) Q_DECL_OVERRIDE;
    bool isShortCircuit() const Q_DECL_OVERRIDE { return true; }

    void _q_send(QMetaObject::Call call, int index, const QVariantList &args) Q_DECL_OVERRIDE;
    QRemoteObjectPendingCall _q_sendWithReply(QMetaObject::Call call, int index, const QVariantList& args) Q_DECL_OVERRIDE;

    QPointer<QRemoteObjectSource> connectionToSource;
};

QT_END_NAMESPACE

#endif
