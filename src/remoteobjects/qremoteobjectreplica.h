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

#ifndef QQREMOTEOBJECTREPLICA_H
#define QQREMOTEOBJECTREPLICA_H

#include <QtRemoteObjects/qtremoteobjectglobal.h>

#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE

class QRemoteObjectPendingCall;
class QRemoteObjectReplicaPrivate;
class QReplicaPrivateInterface;
class QRemoteObjectNode;

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectReplica : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QRemoteObjectNode *node READ node WRITE setNode)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
public:
    enum State {
        Uninitialized,
        Default,
        Valid,
        Suspect,
        SignatureMismatch
    };
    Q_ENUM(State)

public:
    virtual ~QRemoteObjectReplica();

    bool isReplicaValid() const;
    bool waitForSource(int timeout = 30000);
    bool isInitialized() const;
    State state() const;
    QRemoteObjectNode *node() const;
    void setNode(QRemoteObjectNode *node);

Q_SIGNALS:
    void initialized();
    void stateChanged(State state, State oldState);

protected:
    enum ConstructorType {DefaultConstructor, ConstructWithNode};
    explicit QRemoteObjectReplica(ConstructorType t = DefaultConstructor);

    virtual void initialize();
    void send(QMetaObject::Call call, int index, const QVariantList &args);
    QRemoteObjectPendingCall sendWithReply(QMetaObject::Call call, int index, const QVariantList &args);

protected:
    void setProperties(const QVariantList &);
    const QVariant propAsVariant(int i) const;
    void persistProperties(const QString &repName, const QByteArray &repSig, const QVariantList &props) const;
    QVariantList retrieveProperties(const QString &repName, const QByteArray &repSig) const;
    void initializeNode(QRemoteObjectNode *node, const QString &name = QString());
    QSharedPointer<QReplicaPrivateInterface> d_ptr;
private:
    friend class QRemoteObjectNodePrivate;
};

QT_END_NAMESPACE

#endif
