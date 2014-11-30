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

#ifndef QREMOTEOBJECTNODE_H
#define QREMOTEOBJECTNODE_H

#include <QtCore/QSharedPointer>
#include <QtRemoteObjects/qtremoteobjectglobal.h>
#include <QtRemoteObjects/qremoteobjectregistry.h>
#include <QtRemoteObjects/qremoteobjectdynamicreplica.h>

QT_BEGIN_NAMESPACE

class QRemoteObjectReplica;
class QRemoteObjectNodePrivate;
class SourceApiMap;

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectNode
{
public:

    enum ErrorCode{
        NoError,
        RegistryNotAcquired,
        RegistryAlreadyHosted,
        NodeIsNoServer,
        ServerAlreadyCreated,
        UnintendedRegistryHosting,
        OperationNotValidOnClientNode,
        SourceNotRegistered,
        MissingObjectName
    };

    QRemoteObjectNode();
    ~QRemoteObjectNode();

    //TODO maybe set defaults from a #define to allow override?
    static QRemoteObjectNode createHostNode(const QUrl &hostAddress = QUrl(QString::fromLatin1("local:replica")));
    static QRemoteObjectNode createRegistryHostNode(const QUrl &hostAddress = QUrl(QString::fromLatin1("local:registry")));
    static QRemoteObjectNode createNodeConnectedToRegistry(const QUrl &registryAddress = QUrl(QString::fromLatin1("local:registry")));
    static QRemoteObjectNode createHostNodeConnectedToRegistry(const QUrl &hostAddress = QUrl(QString::fromLatin1("local:replica")),
                                                         const QUrl &registryAddress = QUrl(QString::fromLatin1("local:registry")));
    QUrl hostUrl() const;
    bool setHostUrl(const QUrl &hostAddress);
    QUrl registryUrl() const;
    bool setRegistryUrl(const QUrl &registryAddress);
    bool hostRegistry();
    void connect(const QUrl &address=QUrl(QString::fromLatin1("local:replica")));
    const QRemoteObjectRegistry *registry() const;
    template < class ObjectType >
    ObjectType *acquire()
    {
        ObjectType* replica = new ObjectType;
        return qobject_cast< ObjectType* >(acquire(&ObjectType::staticMetaObject, replica));
    }
    QRemoteObjectDynamicReplica *acquire(const QString &name);

    template <template <typename> class ApiDefinition, typename ObjectType>
    bool enableRemoting(ObjectType *object)
    {
        ApiDefinition<ObjectType> *api = new ApiDefinition<ObjectType>;
        return enableRemoting(object, api);
    }
    bool enableRemoting(QObject *object, const QMetaObject *meta = Q_NULLPTR);
    bool disableRemoting(QObject *remoteObject);

    ErrorCode lastError() const;

private:
    QRemoteObjectNode(const QUrl &hostAddress, const QUrl &registryAddress);
    QRemoteObjectReplica *acquire(const QMetaObject *, QRemoteObjectReplica *);
    bool enableRemoting(QObject *object, const SourceApiMap *);

    QSharedPointer<QRemoteObjectNodePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
