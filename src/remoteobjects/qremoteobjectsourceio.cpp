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
#include "qremoteobjectnode_p.h"
#include "qtremoteobjectglobal.h"

#include "qconnection_tcpip_backend_p.h"
#include "qconnection_local_backend_p.h"

#include <QStringList>

QT_BEGIN_NAMESPACE

using namespace QtRemoteObjects;


QRemoteObjectSourceIoAbstract::QRemoteObjectSourceIoAbstract(QObject *parent)
    : QObject(parent)
{

}

QRemoteObjectSourceIo::QRemoteObjectSourceIo(const QUrl &address, QObject *parent)
    : QRemoteObjectSourceIoAbstract(parent)
    , m_server(QtROServerFactory::create(address, this))
{
    if (m_server && m_server->listen(address)) {
        qRODebug(this) << "QRemoteObjectSourceIo is Listening" << address;
    } else {
        qROWarning(this) << "Listen failed for URL:" << address;
        if (m_server)
            qROWarning(this) << m_server->serverError();
        else
            qROWarning(this) << "Most likely an unrecognized scheme was used.";
        return;
    }

    connect(m_server.data(), &QConnectionAbstractServer::newConnection, this, &QRemoteObjectSourceIo::handleConnection);
    connect(&m_serverDelete, static_cast<void (QSignalMapper::*)(QObject *)>(&QSignalMapper::mapped), this, &QRemoteObjectSourceIo::onServerDisconnect);
    connect(&m_serverRead, static_cast<void (QSignalMapper::*)(QObject *)>(&QSignalMapper::mapped), this, &QRemoteObjectSourceIo::onServerRead);
}

QRemoteObjectSourceIoAbstract::~QRemoteObjectSourceIoAbstract()
{
    qDeleteAll(m_remoteObjects.values());
}

QUrl QRemoteObjectSourceIoAbstract::serverAddress() const
{
    return QUrl();
}

bool QRemoteObjectSourceIoAbstract::serverIsNull() const
{
    return true;
}

bool QRemoteObjectSourceIoAbstract::enableRemoting(QObject *object, const QMetaObject *meta, const QString &name, const QString &typeName)
{
    if (m_remoteObjects.contains(name)) {
        qROWarning(this) << "Tried to register QRemoteObjectSource twice" << name;
        return false;
    }

    return enableRemoting(object, new DynamicApiMap(meta, name, typeName));
}

bool QRemoteObjectSourceIoAbstract::enableRemoting(QObject *object, const SourceApiMap *api, QObject *adapter)
{
    const QString name = api->name();
    if (!api->isDynamic() && m_remoteObjects.contains(name)) {
        qROWarning(this) << "Tried to register QRemoteObjectSource twice" << name;
        return false;
    }

    new QRemoteObjectSource(object, api, adapter, this);
    serializeObjectListPacket(m_packet, {QRemoteObjectPackets::ObjectInfo{api->name(), api->typeName()}});
    foreach (ServerIoDevice *conn, connections())
        conn->write(m_packet.array, m_packet.size);
    if (const int count = connections().size())
        qRODebug(this) << "Wrote new QObjectListPacket for" << api->name() << "to" << count << "connections";
    return true;
}

bool QRemoteObjectSourceIoAbstract::disableRemoting(QObject *object)
{
    QRemoteObjectSource *pp = m_objectToSourceMap.take(object);
    if (!pp)
        return false;

    delete pp;
    return true;
}

void QRemoteObjectSourceIoAbstract::onReadData(ServerIoDevice *connection)
{
    QRemoteObjectPacketTypeEnum packetType;

    do {

        if (!connection->read(packetType, m_rxName))
            return;

        using namespace QRemoteObjectPackets;

        switch (packetType) {
        case AddObject:
        {
            bool isDynamic;
            deserializeAddObjectPacket(connection->stream(), isDynamic);
            qRODebug(this) << "AddObject" << m_rxName << isDynamic;
            if (m_remoteObjects.contains(m_rxName)) {
                QRemoteObjectSource *pp = m_remoteObjects[m_rxName];
                pp->addListener(connection, isDynamic);
            } else {
                qROWarning(this) << "Request to attach to non-existent RemoteObjectSource:" << m_rxName;
            }
            break;
        }
        case RemoveObject:
        {
            qRODebug(this) << "RemoveObject" << m_rxName;
            if (m_remoteObjects.contains(m_rxName)) {
                QRemoteObjectSource *pp = m_remoteObjects[m_rxName];
                const int count = pp->removeListener(connection);
                Q_UNUSED(count);
                //TODO - possible to have a timer that closes connections if not reopened within a timeout?
            } else {
                qROWarning(this) << "Request to detach from non-existent RemoteObjectSource:" << m_rxName;
            }
            qRODebug(this) << "RemoveObject finished" << m_rxName;
            break;
        }
        case InvokePacket:
        {
            int call, index, serialId;
            deserializeInvokePacket(connection->stream(), call, index, m_rxArgs, serialId);
            if (m_rxName == QStringLiteral("Registry") && !m_registryMapping.contains(connection)) {
                const QRemoteObjectSourceLocation loc = m_rxArgs.first().value<QRemoteObjectSourceLocation>();
                m_registryMapping[connection] = loc.second.hostUrl;
            }
            if (m_remoteObjects.contains(m_rxName)) {
                QRemoteObjectSource *pp = m_remoteObjects[m_rxName];
                if (call == QMetaObject::InvokeMetaMethod) {
                    const int resolvedIndex = pp->m_api->sourceMethodIndex(index);
                    if (resolvedIndex < 0) { //Invalid index
                        qROWarning(this) << "Invalid method invoke packet received.  Index =" << index <<"which is out of bounds for type"<<m_rxName;
                        //TODO - consider moving this to packet validation?
                        break;
                    }
                    if (pp->m_api->isAdapterMethod(index))
                        qRODebug(this) << "Adapter (method) Invoke-->" << m_rxName << pp->m_adapter->metaObject()->method(resolvedIndex).name();
                    else
                        qRODebug(this) << "Source (method) Invoke-->" << m_rxName << pp->m_object->metaObject()->method(resolvedIndex).name();
                    int typeId = QMetaType::type(pp->m_api->typeName(index).constData());
                    if (!QMetaType(typeId).sizeOf())
                        typeId = QVariant::Invalid;
                    QVariant returnValue(typeId, Q_NULLPTR);
                    pp->invoke(QMetaObject::InvokeMetaMethod, pp->m_api->isAdapterMethod(index), resolvedIndex, m_rxArgs, &returnValue);
                    // send reply if wanted
                    if (serialId >= 0) {
                        serializeInvokeReplyPacket(m_packet, m_rxName, serialId, returnValue);
                        connection->write(m_packet.array, m_packet.size);
                    }
                } else {
                    const int resolvedIndex = pp->m_api->sourcePropertyIndex(index);
                    if (resolvedIndex < 0) {
                        qROWarning(this) << "Invalid property invoke packet received.  Index =" << index <<"which is out of bounds for type"<<m_rxName;
                        //TODO - consider moving this to packet validation?
                        break;
                    }
                    if (pp->m_api->isAdapterProperty(index))
                        qRODebug(this) << "Adapter (write property) Invoke-->" << m_rxName << pp->m_adapter->metaObject()->property(resolvedIndex).name();
                    else
                        qRODebug(this) << "Source (write property) Invoke-->" << m_rxName << pp->m_object->metaObject()->property(resolvedIndex).name();
                    pp->invoke(QMetaObject::WriteProperty, pp->m_api->isAdapterProperty(index), resolvedIndex, m_rxArgs);
                }
            }
            break;
        }
        default:
            qRODebug(this) << "OnReadReady invalid type" << packetType;
        }
    } while (connection->bytesAvailable()); // have bytes left over, so do another iteration
}

void QRemoteObjectSourceIoAbstract::registerSource(QRemoteObjectSource *pp)
{
    Q_ASSERT(pp);
    const QString name = pp->m_api->name();
    const auto type = pp->m_api->typeName();
    m_objectToSourceMap[pp->m_object] = pp;
    m_remoteObjects[name] = pp;
    qRODebug(this) << "Registering" << name;
    notifyObjectAdded(name,type);
}

void QRemoteObjectSourceIoAbstract::unregisterSource(QRemoteObjectSource *pp)
{
    Q_ASSERT(pp);
    const QString name = pp->m_api->name();
    const auto type = pp->m_api->typeName();
    m_objectToSourceMap.remove(pp->m_object);
    m_remoteObjects.remove(name);
    notifyObjectRemoved(name,type);
}

QMap<QString, QRemoteObjectSource *> QRemoteObjectSourceIoAbstract::remoteObjects() const
{
    return m_remoteObjects;
}

void QRemoteObjectSourceIoAbstract::notifyObjectAdded(const QString name, const QString type)
{
    Q_UNUSED(name)
    Q_UNUSED(type)
}

void QRemoteObjectSourceIoAbstract::notifyObjectRemoved(const QString name, const QString type)
{
    Q_UNUSED(name)
    Q_UNUSED(type)
}

bool QRemoteObjectSourceIo::serverIsNull() const
{
    return m_server.isNull();
}

QSet<ServerIoDevice *> QRemoteObjectSourceIo::connections()
{
    return m_connections;
}

void QRemoteObjectSourceIo::notifyObjectAdded(const QString name, const QString type)
{
    emit remoteObjectAdded(qMakePair(name, QRemoteObjectSourceLocationInfo(type, serverAddress())));
}

void QRemoteObjectSourceIo::notifyObjectRemoved(const QString name, const QString type)
{
    emit remoteObjectRemoved(qMakePair(name, QRemoteObjectSourceLocationInfo(type, serverAddress())));
}

void QRemoteObjectSourceIo::onServerDisconnect(QObject *conn)
{
    ServerIoDevice *connection = qobject_cast<ServerIoDevice*>(conn);
    m_connections.remove(connection);

    qRODebug(this) << "OnServerDisconnect";

    Q_FOREACH (QRemoteObjectSource *pp, m_remoteObjects)
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

    onReadData(connection);
}

void QRemoteObjectSourceIo::handleConnection()
{
    qRODebug(this) << "handleConnection" << m_connections;

    ServerIoDevice *conn = m_server->nextPendingConnection();
    m_connections.insert(conn);
    connect(conn, SIGNAL(disconnected()), &m_serverDelete, SLOT(map()));
    m_serverDelete.setMapping(conn, conn);
    connect(conn, SIGNAL(readyRead()), &m_serverRead, SLOT(map()));
    m_serverRead.setMapping(conn, conn);

    QRemoteObjectPackets::ObjectInfoList infos;
    foreach (auto remoteObject, m_remoteObjects) {
        infos << QRemoteObjectPackets::ObjectInfo{remoteObject->m_api->name(), remoteObject->m_api->typeName()};
    }
    serializeObjectListPacket(m_packet, infos);
    conn->write(m_packet.array, m_packet.size);
    qRODebug(this) << "Wrote ObjectList packet from Server" << QStringList(m_remoteObjects.keys());
}

QUrl QRemoteObjectSourceIo::serverAddress() const
{
    return m_server->address();
}

QT_END_NAMESPACE

QRemoteObjectSourceSocketIo::QRemoteObjectSourceSocketIo(QSharedPointer<QIODevice> device, QObject *parent)
    : QRemoteObjectSourceIoAbstract(parent)
{
    m_connection = 0;
    setSocket(device);
}

void QRemoteObjectSourceSocketIo::setSocket(QSharedPointer<QIODevice> device)
{
    const QMetaObject *mo = device.data()->metaObject();
    QString name = QString::fromLocal8Bit(mo->className());
    if(m_connection)
    {
        disconnect(m_connection,&ServerIoDevice::disconnected,this,&QRemoteObjectSourceSocketIo::onConnectionDisconnect);
        disconnect(m_connection,&ServerIoDevice::readyRead,this,&QRemoteObjectSourceSocketIo::onConnectionRead);
        m_connection->deleteLater();
    }

    m_connection = 0;
    if(name == QStringLiteral("QTcpSocket")
            || name == QStringLiteral("QSslSocket"))
    {
        m_connection = new TcpServerIo(qSharedPointerCast<QTcpSocket>(device),this);
    }
    else if(name == QStringLiteral("QLocalSocket"))
    {
        m_connection = new LocalServerIo(qSharedPointerCast<QLocalSocket>(device),this);
    }
    if (!m_connection) {
        qROWarning(this) << "Could not create ServerIoDevice for socket. Invalid device?" << name;
        return;
    }

    connect(m_connection,&ServerIoDevice::disconnected,this,&QRemoteObjectSourceSocketIo::onConnectionDisconnect);
    connect(m_connection,&ServerIoDevice::readyRead,this,&QRemoteObjectSourceSocketIo::onConnectionRead);

    QRemoteObjectPackets::ObjectInfoList infos;
    foreach (auto remoteObject, m_remoteObjects) {
        infos << QRemoteObjectPackets::ObjectInfo{remoteObject->m_api->name(), remoteObject->m_api->typeName()};
    }
    serializeObjectListPacket(m_packet, infos);
    m_connection->write(m_packet.array, m_packet.size);
    qRODebug(this) << "Wrote ObjectList packet from Server" << QStringList(m_remoteObjects.keys());
}

QSet<ServerIoDevice *> QRemoteObjectSourceSocketIo::connections()
{
    QSet<ServerIoDevice*> ret;
    if(m_connection)
    {
        ret << m_connection;
        return ret;
    }
    return ret;
}

void QRemoteObjectSourceSocketIo::onConnectionDisconnect()
{
    Q_FOREACH (QRemoteObjectSource *pp, m_remoteObjects)
        pp->removeListener(m_connection);

    m_connection->close();
    disconnect(m_connection,&ServerIoDevice::disconnected,this,&QRemoteObjectSourceSocketIo::onConnectionDisconnect);
    disconnect(m_connection,&ServerIoDevice::readyRead,this,&QRemoteObjectSourceSocketIo::onConnectionRead);
    m_connection->deleteLater();
    m_connection = 0;
}

void QRemoteObjectSourceSocketIo::onConnectionRead()
{
    onReadData(m_connection);
}
