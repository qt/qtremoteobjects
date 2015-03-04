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

#include "qremoteobjectsourceio_p.h"

#include "qremoteobjectpacket_p.h"
#include "qremoteobjectsource_p.h"
#include "qtremoteobjectglobal.h"

#include <QStringList>

QT_BEGIN_NAMESPACE

QRemoteObjectSourceIo::QRemoteObjectSourceIo(const QUrl &address)
    : m_server(m_factory.createServer(address, this))
{
    if (m_server->listen(address)) {
        qCDebug(QT_REMOTEOBJECT) << "QRemoteObjectSourceIo is Listening" << address;
    } else {
        qCDebug(QT_REMOTEOBJECT) << "Listen failed";
        qCDebug(QT_REMOTEOBJECT) << address;
        qCDebug(QT_REMOTEOBJECT) << m_server->serverError();
    }

    connect(m_server.data(), &QConnectionAbstractServer::newConnection, this, &QRemoteObjectSourceIo::handleConnection);
    connect(&m_serverDelete, static_cast<void (QSignalMapper::*)(QObject *)>(&QSignalMapper::mapped), this, &QRemoteObjectSourceIo::onServerDisconnect);
    connect(&m_serverRead, static_cast<void (QSignalMapper::*)(QObject *)>(&QSignalMapper::mapped), this, &QRemoteObjectSourceIo::onServerRead);
    connect(&m_remoteObjectDestroyed, static_cast<void (QSignalMapper::*)(const QString &)>(&QSignalMapper::mapped), this, &QRemoteObjectSourceIo::clearRemoteObjectSource);
}

QRemoteObjectSourceIo::~QRemoteObjectSourceIo()
{
    Q_FOREACH (QRemoteObjectSourcePrivate *pp, m_remoteObjects) {
        disableRemoting(pp);
    }
}

bool QRemoteObjectSourceIo::enableRemoting(QObject *object, const QMetaObject *meta, const QString &name)
{
    if (m_remoteObjects.contains(name)) {
        qCWarning(QT_REMOTEOBJECT) << "Tried to register QRemoteObjectSource twice" << name;
        return false;
    }

    return enableRemoting(object, new DynamicApiMap(object, meta, name));
}

bool QRemoteObjectSourceIo::enableRemoting(QObject *object, const SourceApiMap *api, QObject *adapter)
{
    const QString name = api->name();
    if (!api->isDynamic() && m_remoteObjects.contains(name)) {
        qCWarning(QT_REMOTEOBJECT) << "Tried to register QRemoteObjectSource twice" << name;
        return false;
    }

    QRemoteObjectSourcePrivate *pp = new QRemoteObjectSourcePrivate(object, api, adapter);
    qCDebug(QT_REMOTEOBJECT) << "Registering" << name;

    m_remoteObjects[name] = pp;
    connect(pp, SIGNAL(destroyed()), &m_remoteObjectDestroyed, SLOT(map()));
    m_remoteObjectDestroyed.setMapping(pp, name);
    emit remoteObjectAdded(qMakePair(name, m_server->address()));

    return true;
}

bool QRemoteObjectSourceIo::disableRemoting(QRemoteObjectSourcePrivate *pp)
{
    const QString name = pp->m_api->name();
    clearRemoteObjectSource(name);
    pp->setParent(Q_NULLPTR);
    delete pp;

    return true;
}

void QRemoteObjectSourceIo::onServerDisconnect(QObject *conn)
{
    ServerIoDevice *connection = qobject_cast<ServerIoDevice*>(conn);
    m_connections.remove(connection);

    qCDebug(QT_REMOTEOBJECT) << "OnServerDisconnect";

    Q_FOREACH (QRemoteObjectSourcePrivate *pp, m_remoteObjects)
        pp->removeListener(connection);

    const QUrl location = m_registryMapping.value(connection);
    emit serverRemoved(location);
    m_registryMapping.remove(connection);
    connection->close();
    connection->deleteLater();
}

void QRemoteObjectSourceIo::onServerRead(QObject *conn)
{
    // Assert the invariant here conn is of type QIODevice
    ServerIoDevice *connection = qobject_cast<ServerIoDevice*>(conn);

    do {

        if (!connection->read())
            return;

        using namespace QRemoteObjectPackets;

        const QRemoteObjectPacket* packet = connection->packet();
        switch (packet->id) {
        case QRemoteObjectPacket::AddObject:
        {
            const QAddObjectPacket *p = static_cast<const QAddObjectPacket *>(packet);
            const QString name = p->name;
            qCDebug(QT_REMOTEOBJECT) << "AddObject" << name << p->isDynamic;
            if (m_remoteObjects.contains(name)) {
                QRemoteObjectSourcePrivate *pp = m_remoteObjects[name];
                pp->addListener(connection, p->isDynamic);
            } else {
                qCWarning(QT_REMOTEOBJECT) << "Request to attach to non-existent RemoteObjectSource:" << name;
            }
            break;
        }
        case QRemoteObjectPacket::RemoveObject:
        {
            const QRemoveObjectPacket *p = static_cast<const QRemoveObjectPacket *>(packet);
            const QString name = p->name;
            qCDebug(QT_REMOTEOBJECT) << "RemoveObject" << name;
            if (m_remoteObjects.contains(name)) {
                QRemoteObjectSourcePrivate *pp = m_remoteObjects[name];
                const int count = pp->removeListener(connection);
                Q_UNUSED(count);
                //TODO - possible to have a timer that closes connections if not reopened within a timeout?
            } else {
                qCWarning(QT_REMOTEOBJECT) << "Request to detach from non-existent RemoteObjectSource:" << name;
            }
            qCDebug(QT_REMOTEOBJECT) << "RemoveObject finished" << name;
            break;
        }
        case QRemoteObjectPacket::InvokePacket:
        {
            const QInvokePacket *p = static_cast<const QInvokePacket *>(packet);
            const QString name = p->name;
            if (name == QStringLiteral("Registry") && !m_registryMapping.contains(connection)) {
                const QRemoteObjectSourceLocation loc = p->args.first().value<QRemoteObjectSourceLocation>();
                m_registryMapping[connection] = loc.second;
            }
            if (m_remoteObjects.contains(name)) {
                QRemoteObjectSourcePrivate *pp = m_remoteObjects[name];
                if (p->call == QMetaObject::InvokeMetaMethod) {
                    const int resolvedIndex = pp->m_api->sourceMethodIndex(p->index);
                    if (resolvedIndex < 0) { //Invalid index
                        qCWarning(QT_REMOTEOBJECT) << "Invalid method invoke packet received.  Index =" << p->index <<"which is out of bounds for type"<<name;
                        //TODO - consider moving this to packet validation?
                        break;
                    }
                    if (pp->m_api->isAdapterMethod(p->index))
                        qCDebug(QT_REMOTEOBJECT) << "Adapter (method) Invoke-->" << name << pp->m_adapter->metaObject()->method(resolvedIndex).name();
                    else
                        qCDebug(QT_REMOTEOBJECT) << "Source (method) Invoke-->" << name << pp->m_object->metaObject()->method(resolvedIndex).name();
                    int typeId = QMetaType::type(pp->m_api->typeName(p->index).constData());
                    if (!QMetaType(typeId).sizeOf())
                        typeId = QVariant::Invalid;
                    QVariant returnValue(typeId, Q_NULLPTR);
                    pp->invoke(QMetaObject::InvokeMetaMethod, pp->m_api->isAdapterMethod(p->index), resolvedIndex, p->args, &returnValue);
                    // send reply if wanted
                    if (p->serialId >= 0) {
                        QRemoteObjectPackets::QInvokeReplyPacket replyPacket(name, p->serialId, returnValue);
                        connection->write(replyPacket.serialize());
                    }
                } else {
                    const int resolvedIndex = pp->m_api->sourcePropertyIndex(p->index);
                    if (resolvedIndex < 0) {
                        qCWarning(QT_REMOTEOBJECT) << "Invalid property invoke packet received.  Index =" << p->index <<"which is out of bounds for type"<<name;
                        //TODO - consider moving this to packet validation?
                        break;
                    }
                    if (pp->m_api->isAdapterProperty(p->index))
                        qCDebug(QT_REMOTEOBJECT) << "Adapter (write property) Invoke-->" << name << pp->m_adapter->metaObject()->property(resolvedIndex).name();
                    else
                        qCDebug(QT_REMOTEOBJECT) << "Source (write property) Invoke-->" << name << pp->m_object->metaObject()->property(resolvedIndex).name();
                    pp->invoke(QMetaObject::WriteProperty, pp->m_api->isAdapterProperty(p->index), resolvedIndex, p->args);
                }
            }
            break;
        }
        default:
            qCDebug(QT_REMOTEOBJECT) << "OnReadReady invalid type" << packet->id;
        }
    } while (connection->bytesAvailable()); // have bytes left over, so do another iteration
}

void QRemoteObjectSourceIo::handleConnection()
{
    qCDebug(QT_REMOTEOBJECT) << "handleConnection" << m_connections;

    ServerIoDevice *conn = m_server->nextPendingConnection();
    m_connections.insert(conn);
    connect(conn, SIGNAL(disconnected()), &m_serverDelete, SLOT(map()));
    m_serverDelete.setMapping(conn, conn);
    connect(conn, SIGNAL(readyRead()), &m_serverRead, SLOT(map()));
    m_serverRead.setMapping(conn, conn);

    QRemoteObjectPackets::QObjectListPacket p(QStringList(m_remoteObjects.keys()));
    conn->write(p.serialize());
    qCDebug(QT_REMOTEOBJECT) << "Wrote ObjectList packet from Server" << QStringList(m_remoteObjects.keys());
}

void QRemoteObjectSourceIo::clearRemoteObjectSource(const QString &name)
{
    m_remoteObjects.remove(name);
    emit remoteObjectRemoved(qMakePair(name, serverAddress()));
}

QUrl QRemoteObjectSourceIo::serverAddress() const
{
    return m_server->address();
}

QT_END_NAMESPACE
