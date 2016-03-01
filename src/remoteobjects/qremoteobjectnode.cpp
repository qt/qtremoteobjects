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

#include "private/qmetaobjectbuilder_p.h"

#include "qremoteobjectnode.h"
#include "qremoteobjectnode_p.h"

#include "qremoteobjectregistry.h"
#include "qremoteobjectdynamicreplica.h"
#include "qremoteobjectpacket_p.h"
#include "qremoteobjectregistrysource_p.h"
#include "qremoteobjectreplica_p.h"
#include "qremoteobjectsource_p.h"
#include "qremoteobjectabstractitemreplica_p.h"
#include "qremoteobjectabstractitemadapter_p.h"
#include <QAbstractItemModel>

QT_BEGIN_NAMESPACE

using namespace QtRemoteObjects;

static QString name(const QMetaObject * const mobj)
{
    const int ind = mobj->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE);
    return ind >= 0 ? QString::fromLatin1(mobj->classInfo(ind).value()) : QString();
}

template <typename K, typename V, typename Query>
bool map_contains(const QMap<K,V> &map, const Query &key, typename QMap<K,V>::const_iterator &result)
{
    const typename QMap<K,V>::const_iterator it = map.find(key);
    if (it == map.end())
        return false;
    result = it;
    return true;
}

QRemoteObjectNodePrivate::QRemoteObjectNodePrivate()
    : QObjectPrivate()
    , registry(Q_NULLPTR)
    , retryInterval(250)
    , m_lastError(QRemoteObjectNode::NoError)
{ }

QRemoteObjectNodePrivate::~QRemoteObjectNodePrivate()
{ }

QRemoteObjectSourceLocations QRemoteObjectNodePrivate::remoteObjectAddresses() const
{
    if (registry)
        return registry->sourceLocations();
    return QRemoteObjectSourceLocations();
}

QRemoteObjectSourceLocations QRemoteObjectRegistryHostPrivate::remoteObjectAddresses() const
{
    if (registrySource)
        return registrySource->sourceLocations();
    return QRemoteObjectSourceLocations();
}

void QRemoteObjectNode::timerEvent(QTimerEvent*)
{
    Q_D(QRemoteObjectNode);
    Q_FOREACH (ClientIoDevice *conn, d->pendingReconnect) {
        if (conn->isOpen())
            d->pendingReconnect.remove(conn);
        else
            conn->connectToServer();
    }

    if (d->pendingReconnect.isEmpty())
        d->reconnectTimer.stop();

    qRODebug(this) << "timerEvent" << d->pendingReconnect.size();
}

/*!
    \internal In order to create a Replica from QML, it is necessary for the
    Replica to have a default constructor.  In order for it to be properly
    constructed, there needs to be a way to associate the Replica with a
    Node and start the Replica initialization.  Thus we need a public
    method on Node to facilitate that.  That's initializeReplica.
*/
void QRemoteObjectNode::initializeReplica(QRemoteObjectReplica *instance, const QString &name)
{
    Q_D(QRemoteObjectNode);
    if (instance->inherits("QRemoteObjectDynamicReplica")) {
        d->setReplicaPrivate(Q_NULLPTR, instance, name);
    } else {
        const QMetaObject *meta = instance->metaObject();
        d->setReplicaPrivate(meta, instance, name.isEmpty() ? ::name(meta) : name);
    }
}

void QRemoteObjectNodePrivate::setReplicaPrivate(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name)
{
    qROPrivDebug() << "Starting setReplicaPrivate for" << name;
    isInitialized.storeRelease(1);
    openConnectionIfNeeded(name);
    QMutexLocker locker(&mutex);
    if (hasInstance(name)) {
        qCDebug(QT_REMOTEOBJECT)<<"setReplicaPrivate - using existing instance";
        QSharedPointer<QRemoteObjectReplicaPrivate> rep = qSharedPointerCast<QRemoteObjectReplicaPrivate>(replicas.value(name).toStrongRef());
        Q_ASSERT(rep);
        instance->d_ptr = rep;
        rep->configurePrivate(instance);
    } else {
        instance->d_ptr.reset(handleNewAcquire(meta, instance, name));
        instance->initialize();
        replicas.insert(name, instance->d_ptr.toWeakRef());
        qROPrivDebug() << "setReplicaPrivate - Created new instance" << name<<remoteObjectAddresses();
    }
}

/*!
    Returns a pointer to the Node's \l {QRemoteObjectRegistry}, if the Node is using the Registry feature; otherwise it returns 0.
*/
const QRemoteObjectRegistry *QRemoteObjectNode::registry() const
{
    Q_D(const QRemoteObjectNode);
    return d->registry;
}

void QRemoteObjectNodePrivate::connectReplica(QObject *object, QRemoteObjectReplica *instance)
{
    int nConnections = 0;
    const QMetaObject *us = instance->metaObject();
    const QMetaObject *them = object->metaObject();

    static const int memberOffset = QRemoteObjectReplica::staticMetaObject.methodCount();
    for (int idx = memberOffset; idx < us->methodCount(); ++idx) {
        const QMetaMethod mm = us->method(idx);

        qROPrivDebug() << idx << mm.name();
        if (mm.methodType() != QMetaMethod::Signal)
            continue;

        // try to connect to a signal on the parent that has the same method signature
        QByteArray sig = QMetaObject::normalizedSignature(mm.methodSignature().constData());
        qROPrivDebug() << sig;
        if (them->indexOfSignal(sig.constData()) == -1)
            continue;

        sig.prepend(QSIGNAL_CODE + '0');
        const char * const csig = sig.constData();
        const bool res = QObject::connect(object, csig, instance, csig);
        Q_UNUSED(res);
        ++nConnections;

        qROPrivDebug() << sig << res;
    }

    qROPrivDebug() << "# connections =" << nConnections;
}

void QRemoteObjectNodePrivate::openConnectionIfNeeded(const QString &name)
{
    qROPrivDebug() << Q_FUNC_INFO << name << this;
    if (!remoteObjectAddresses().contains(name)) {
        qROPrivDebug() << name << "not available - available addresses:" << remoteObjectAddresses();
        return;
    }

    if (initConnection(remoteObjectAddresses()[name]))
        qROPrivDebug() << "openedConnection" << remoteObjectAddresses()[name];
    else
        qROPrivWarning() << "failed to open connection to" << name;
}

bool QRemoteObjectNodePrivate::initConnection(const QUrl &address)
{
    Q_Q(QRemoteObjectNode);
    if (requestedUrls.contains(address)) {
        qROPrivWarning() << "Connection already initialized for " << address.toString();
        return false;
    }

    requestedUrls.insert(address);

    ClientIoDevice *connection = QtROClientFactory::create(address, q);
    if (!connection) {
        qROPrivWarning() << "Could not create ClientIoDevice for client. Invalid url/scheme provided?" << address;
        return false;
    }

    qROPrivDebug() << "Replica Connection isValid" << connection->isOpen();
    QObject::connect(connection, SIGNAL(shouldReconnect(ClientIoDevice*)), q, SLOT(onShouldReconnect(ClientIoDevice*)));
    connection->connectToServer();
    QObject::connect(connection, SIGNAL(readyRead()), &clientRead, SLOT(map()));
    clientRead.setMapping(connection, connection);
    return true;
}

bool QRemoteObjectNodePrivate::hasInstance(const QString &name)
{
    if (!replicas.contains(name))
        return false;

    QSharedPointer<QReplicaPrivateInterface> rep = replicas.value(name).toStrongRef();
    if (!rep) { //already deleted
        replicas.remove(name);
        return false;
    }

    return true;
}

void QRemoteObjectNodePrivate::onRemoteObjectSourceAdded(const QRemoteObjectSourceLocation &entry)
{
    qROPrivDebug() << "onRemoteObjectSourceAdded" << entry << replicas << replicas.contains(entry.first);
    if (!entry.first.isEmpty()) {
        QRemoteObjectSourceLocations locs = registry->sourceLocations();
        locs[entry.first] = entry.second;
        //TODO Is there a way to extend QRemoteObjectSourceLocations in place?
        registry->setProperty(0, QVariant::fromValue(locs));
        qROPrivDebug() << "onRemoteObjectSourceAdded, now locations =" << locs;
    }
    if (replicas.contains(entry.first)) //We have a replica waiting on this remoteObject
    {
        QSharedPointer<QReplicaPrivateInterface> rep = replicas.value(entry.first).toStrongRef();
        if (!rep) { //replica has been deleted, remove from list
            replicas.remove(entry.first);
            return;
        }

        initConnection(entry.second);

        qROPrivDebug() << "Called initConnection due to new RemoteObjectSource added via registry" << entry.first;
    }
}

void QRemoteObjectNodePrivate::onRemoteObjectSourceRemoved(const QRemoteObjectSourceLocation &entry)
{
    if (!entry.first.isEmpty()) {
        QRemoteObjectSourceLocations locs = registry->sourceLocations();
        locs.remove(entry.first);
        registry->setProperty(0, QVariant::fromValue(locs));
    }
}

void QRemoteObjectNodePrivate::onRegistryInitialized()
{
    qROPrivDebug() << "Registry Initialized" << remoteObjectAddresses();

    QHashIterator<QString, QUrl> i(remoteObjectAddresses());
    while (i.hasNext()) {
        i.next();
        if (replicas.contains(i.key())) //We have a replica waiting on this remoteObject
        {
            QSharedPointer<QReplicaPrivateInterface> rep = replicas.value(i.key()).toStrongRef();
            if (rep && !requestedUrls.contains(i.value()))
                initConnection(i.value());
            else if (!rep) //replica has been deleted, remove from list
                replicas.remove(i.key());

            continue;
        }
    }
}

void QRemoteObjectNodePrivate::onShouldReconnect(ClientIoDevice *ioDevice)
{
    Q_Q(QRemoteObjectNode);
    pendingReconnect.insert(ioDevice);

    Q_FOREACH (const QString &remoteObject, ioDevice->remoteObjects()) {
        connectedSources.remove(remoteObject);
        ioDevice->removeSource(remoteObject);
        if (replicas.contains(remoteObject)) { //We have a replica waiting on this remoteObject
            QSharedPointer<QConnectedReplicaPrivate> rep = qSharedPointerCast<QConnectedReplicaPrivate>(replicas.value(remoteObject).toStrongRef());
            if (rep && !rep->connectionToSource.isNull()) {
                rep->setDisconnected();
            } else if (!rep) {
                replicas.remove(remoteObject);
            }
        }
    }
    if (!reconnectTimer.isActive()) {
        reconnectTimer.start(retryInterval, q);
        qROPrivDebug() << "Starting reconnect timer";
    }
}

//This version of handleNewAcquire creates a QConnectedReplica. If this is a
//Host Node, the QRemoteObjectHostBasePrivate overload is called instead.
QReplicaPrivateInterface *QRemoteObjectNodePrivate::handleNewAcquire(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name)
{
    Q_Q(QRemoteObjectNode);
    QConnectedReplicaPrivate *rp = new QConnectedReplicaPrivate(name, meta, q);
    rp->configurePrivate(instance);
    if (connectedSources.contains(name)) { //Either we have a peer connections, or existing connection via registry
        rp->setConnection(connectedSources[name]);
    } else if (remoteObjectAddresses().contains(name)) { //No existing connection, but we know we can connect via registry
        initConnection(remoteObjectAddresses()[name]); //This will try the connection, and if successful, the remoteObjects will be sent
                                              //The link to the replica will be handled then
    }
    return rp;
}

//Host Nodes can use the more efficient InProcess Replica if we (this Node) hold the Source for the
//requested Replica.  If not, fall back to the Connected Replica case.
QReplicaPrivateInterface *QRemoteObjectHostBasePrivate::handleNewAcquire(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name)
{
    QMap<QString, QRemoteObjectSource*>::const_iterator mapIt;
    if (map_contains(remoteObjectIo->m_remoteObjects, name, mapIt)) {
        Q_Q(QRemoteObjectHostBase);
        QInProcessReplicaPrivate *rp = new QInProcessReplicaPrivate(name, meta, q);
        rp->configurePrivate(instance);
        connectReplica(mapIt.value()->m_object, instance);
        rp->connectionToSource = mapIt.value();
        return rp;
    }
    return QRemoteObjectNodePrivate::handleNewAcquire(meta, instance, name);
}

void QRemoteObjectNodePrivate::onClientRead(QObject *obj)
{
    using namespace QRemoteObjectPackets;
    ClientIoDevice *connection = qobject_cast<ClientIoDevice*>(obj);
    QRemoteObjectPacketTypeEnum packetType;
    Q_ASSERT(connection);

    do {

        if (!connection->read(packetType, m_rxName))
            return;

        switch (packetType) {
        case ObjectList:
        {
            deserializeObjectListPacket(connection->stream(), m_rxObjects);
            const QSet<QString> newObjects = m_rxObjects.toSet();
            qROPrivDebug() << "newObjects:" << newObjects;
            Q_FOREACH (const QString &remoteObject, newObjects) {
                qROPrivDebug() << "  connectedSources.contains("<<remoteObject<<")"<<connectedSources.contains(remoteObject)<<replicas.contains(remoteObject);
                if (!connectedSources.contains(remoteObject)) {
                    connectedSources[remoteObject] = connection;
                    connection->addSource(remoteObject);
                    if (replicas.contains(remoteObject)) //We have a replica waiting on this remoteObject
                    {
                        QSharedPointer<QConnectedReplicaPrivate> rep = qSharedPointerCast<QConnectedReplicaPrivate>(replicas.value(remoteObject).toStrongRef());
                        if (rep && rep->connectionToSource.isNull())
                        {
                            qROPrivDebug() << "Test" << remoteObject<<replicas.keys();
                            qROPrivDebug() << rep;
                            rep->setConnection(connection);
                        } else if (!rep) { //replica has been deleted, remove from list
                            replicas.remove(remoteObject);
                        }

                        continue;
                    }
                }
            }
            break;
        }
        case InitPacket:
        {
            qROPrivDebug() << "InitObject-->" << m_rxName << this;
            QSharedPointer<QConnectedReplicaPrivate> rep = qSharedPointerCast<QConnectedReplicaPrivate>(replicas.value(m_rxName).toStrongRef());
            //Use m_rxArgs (a QVariantList to hold the properties QVariantList)
            deserializeInitPacket(connection->stream(), m_rxArgs);
            if (rep)
            {
                rep->initialize(m_rxArgs);
            } else { //replica has been deleted, remove from list
                replicas.remove(m_rxName);
            }
            break;
        }
        case InitDynamicPacket:
        {
            qROPrivDebug() << "InitObject-->" << m_rxName << this;
            QMetaObjectBuilder builder;
            builder.setClassName("QRemoteObjectDynamicReplica");
            builder.setSuperClass(&QRemoteObjectReplica::staticMetaObject);
            builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
            deserializeInitDynamicPacket(connection->stream(), builder, m_rxArgs);
            QSharedPointer<QConnectedReplicaPrivate> rep = qSharedPointerCast<QConnectedReplicaPrivate>(replicas.value(m_rxName).toStrongRef());
            if (rep)
            {
                rep->initializeMetaObject(builder, m_rxArgs);
            } else { //replica has been deleted, remove from list
                replicas.remove(m_rxName);
            }
            break;
        }
        case RemoveObject:
        {
            qROPrivDebug() << "RemoveObject-->" << m_rxName << this;
            connectedSources.remove(m_rxName);
            connection->removeSource(m_rxName);
            if (replicas.contains(m_rxName)) { //We have a replica waiting on this remoteObject
                QSharedPointer<QConnectedReplicaPrivate> rep = qSharedPointerCast<QConnectedReplicaPrivate>(replicas.value(m_rxName).toStrongRef());
                if (rep && !rep->connectionToSource.isNull()) {
                    rep->connectionToSource.clear();
                    if (rep->isReplicaValid()) {
                        //Changed from receiving to not receiving
                        rep->emitValidChanged();
                    }
                } else if (!rep) {
                    replicas.remove(m_rxName);
                }
            }
            break;
        }
        case PropertyChangePacket:
        {
            int propertyIndex;
            deserializePropertyChangePacket(connection->stream(), propertyIndex, m_rxValue);
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = qSharedPointerCast<QRemoteObjectReplicaPrivate>(replicas.value(m_rxName).toStrongRef());
            if (rep)
            {
                const QMetaProperty property = rep->m_metaObject->property(propertyIndex + rep->m_metaObject->propertyOffset());
                rep->setProperty(propertyIndex, deserializedProperty(m_rxValue, property));
            } else { //replica has been deleted, remove from list
                replicas.remove(m_rxName);
            }
            break;
        }
        case InvokePacket:
        {
            int call, index, serialId;
            deserializeInvokePacket(connection->stream(), call, index, m_rxArgs, serialId);
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = qSharedPointerCast<QRemoteObjectReplicaPrivate>(replicas.value(m_rxName).toStrongRef());
            if (rep)
            {
                static QVariant null(QMetaType::QObjectStar, (void*)0);

                // Qt usually supports 9 arguments, so ten should be usually safe
                QVarLengthArray<void*, 10> param(m_rxArgs.size() + 1);
                param[0] = null.data(); //Never a return value
                for (int i = 0; i < m_rxArgs.size(); i++) {
                    param[i + 1] = const_cast<void *>(m_rxArgs[i].data());
                }
                qROPrivDebug() << "Replica Invoke-->" << m_rxName << rep->m_metaObject->method(index+rep->m_signalOffset).name() << index << rep->m_signalOffset;
                QMetaObject::activate(rep.data(), rep->metaObject(), index+rep->m_signalOffset, param.data());
            } else { //replica has been deleted, remove from list
                replicas.remove(m_rxName);
            }
            break;
        }
        case InvokeReplyPacket:
        {
            int ackedSerialId;
            deserializeInvokeReplyPacket(connection->stream(), ackedSerialId, m_rxValue);
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = qSharedPointerCast<QRemoteObjectReplicaPrivate>(replicas.value(m_rxName).toStrongRef());
            if (rep) {
                qROPrivDebug() << "Received InvokeReplyPacket ack'ing serial id:" << ackedSerialId;
                rep->notifyAboutReply(ackedSerialId, m_rxValue);
            } else { //replica has been deleted, remove from list
                replicas.remove(m_rxName);
            }
            break;
        }
        case AddObject:
        case Invalid:
            qROPrivWarning() << "Unexpected packet received";
        }
    } while (connection->bytesAvailable()); // have bytes left over, so do another iteration
}

/*!
    \class QRemoteObjectNode
    \inmodule QtRemoteObjects
    \brief A Node on a Qt Remote Objects network

    The QRemoteObjectNode class provides the entry point to a QtRemoteObjects
    network. A network can be as simple as two Nodes, or an arbitrarily complex
    set of processes and devices.

    A QRemoteObjectNode that wants to export QRemoteObjectSource objects to the
    network must be a Host Node. Such a Node can be connected to by other
    Nodes. Nodes may connect to each other directly using \l connect, or they
    can use the QRemoteObjectRegistry to simplify connections.

    The QRemoteObjectRegistry is a special Replica available to every Node that
    connects to the Registry Url. It knows how to connect to every
    QRemoteObjectSource object on the network.
*/

/*!
    \enum QRemoteObjectNode::ErrorCode

    This enum type specifies the various error codes associated with QRemoteObjectNode errors:

    \value NoError No error.
    \value RegistryNotAcquired The registry could not be acquired.
    \value RegistryAlreadyHosted The registry is already defined and hosting Sources.
    \value NodeIsNoServer The given QRemoteObjectNode is not a host node.
    \value ServerAlreadyCreated The host node has already been initialized.
    \value UnintendedRegistryHosting An attempt was made to create a host QRemoteObjectNode and connect to itself as the registry.
    \value OperationNotValidOnClientNode The attempted operation is not valid on a client QRemoteObjectNode.
    \value SourceNotRegistered The given QRemoteObjectSource is not registered on this node.
    \value MissingObjectName The given QObject does not have objectName() set.
*/

/*!
    \fn ObjectType *QRemoteObjectNode::acquire()

    Returns a pointer to a Replica of type ObjectType (which is a template
    parameter and must inherit from \l QRemoteObjectReplica). That is, the it
    must be a \l {repc} generated type.
*/

void QRemoteObjectNodePrivate::initialize()
{
    Q_Q(QRemoteObjectNode);
    qRegisterMetaType<QRemoteObjectNode *>();
    qRegisterMetaTypeStreamOperators<QVector<int> >();
    QObject::connect(&clientRead, SIGNAL(mapped(QObject*)), q, SLOT(onClientRead(QObject*)));
}

/*!
    Default constructor for QRemoteObjectNode. A Node constructed in this
    manner can not be connected to, and thus can not expose Source objects on
    the network. It also will not include a \l QRemoteObjectRegistry.

    \sa connectToNode()
*/
QRemoteObjectNode::QRemoteObjectNode(QObject *parent)
    : QObject(*new QRemoteObjectNodePrivate, parent)
{
    Q_D(QRemoteObjectNode);
    d->initialize();
}

/*!
    Registry version of QRemoteObjectNode. A Node constructed in this manner
    can not be connected to, and thus can not expose Source objects on the
    network. Finding and connecting to other (Host) Nodes is handled by the
    QRemoteObjectRegistry specified by \a registryAddress.

    \sa connectToNode(), setRegistryUrl(), QRemoteObjectHost()
*/
QRemoteObjectNode::QRemoteObjectNode(const QUrl &registryAddress, QObject *parent)
    : QObject(*new QRemoteObjectNodePrivate, parent)
{
    Q_D(QRemoteObjectNode);
    d->initialize();
    setRegistryUrl(registryAddress);
}

QRemoteObjectNode::QRemoteObjectNode(QRemoteObjectNodePrivate &dptr, QObject *parent)
    : QObject(dptr, parent)
{
    Q_D(QRemoteObjectNode);
    d->initialize();
}

/*!
    \internal This is a base class for both QRemoteObjectHost and
    QRemoteObjectRegistryHost to provide the shared features/functions for
    sharing \s Source objects.
*/
QRemoteObjectHostBase::QRemoteObjectHostBase(QRemoteObjectHostBasePrivate &d, QObject *parent)
    : QRemoteObjectNode(d, parent)
{ }

/*!
    Constructs a new QRemoteObjectHost Node (i.e., a Node that supports
    exposing \l Source objects on the QtRO network). This constructor is meant
    specific to support QML in the future as it will not be available to
    connect to until \l setHostUrl() is called.

    \sa setHostUrl(), setRegistryUrl()
*/
QRemoteObjectHost::QRemoteObjectHost(QObject *parent)
    : QRemoteObjectHostBase(*new QRemoteObjectHostPrivate, parent)
{ }

/*!
    Constructs a new QRemoteObjectHost Node (i.e., a Node that supports
    exposing \l Source objects on the QtRO network) with address \a address. If
    set, \a registryAddress will be used to connect to the \l
    QRemoteObjectRegistry at the provided address.

    \sa setHostUrl(), setRegistryUrl()
*/
QRemoteObjectHost::QRemoteObjectHost(const QUrl &address, const QUrl &registryAddress, QObject *parent)
    : QRemoteObjectHostBase(*new QRemoteObjectHostPrivate, parent)
{
    if (!address.isEmpty()) {
        if (!setHostUrl(address))
            return;
    }

    if (!registryAddress.isEmpty())
        setRegistryUrl(registryAddress);
}

/*!
    Constructs a new QRemoteObjectHost Node (i.e., a Node that supports
    exposing \l Source objects on the QtRO network) with address \a
    hostAddress. This overload is provided as a convenience for specifying a
    QObject parent without providing a registry address.

    \sa setHostUrl(), setRegistryUrl()
*/
QRemoteObjectHost::QRemoteObjectHost(const QUrl &address, QObject *parent)
    : QRemoteObjectHostBase(*new QRemoteObjectHostPrivate, parent)
{
    if (!address.isEmpty())
        setHostUrl(address);
}

QRemoteObjectHost::QRemoteObjectHost(QRemoteObjectHostPrivate &d, QObject *parent)
    : QRemoteObjectHostBase(d, parent)
{ }

QRemoteObjectHost::~QRemoteObjectHost() {}

/*!
    Constructs a new QRemoteObjectRegistryHost Node. RegistryHost Nodes have
    the same functionality as \l QRemoteObjectHost Nodes, except rather than
    being able to connect to a \l QRemoteObjectRegistry, the provided Host QUrl
    (\a registryAddress) becomes the address of the registry for other Nodes to
    connect to.
*/
QRemoteObjectRegistryHost::QRemoteObjectRegistryHost(const QUrl &registryAddress, QObject *parent)
    : QRemoteObjectHostBase(*new QRemoteObjectRegistryHostPrivate, parent)
{
    if (registryAddress.isEmpty())
        return;

    setRegistryUrl(registryAddress);
}

QRemoteObjectRegistryHost::QRemoteObjectRegistryHost(QRemoteObjectRegistryHostPrivate &d, QObject *parent)
    : QRemoteObjectHostBase(d, parent)
{ }

QRemoteObjectRegistryHost::~QRemoteObjectRegistryHost() {}

/*!
    Destructor for QRemoteObjectNode.
*/
QRemoteObjectNode::~QRemoteObjectNode()
{ }

/*!
    Sets \a name as the internal name for this Node.  This
    is then output as part of the logging (if enabled).
    This is primarily useful if you merge log data from multiple nodes.
*/
void QRemoteObjectNode::setName(const QString &name)
{
    setObjectName(name);
}

void QRemoteObjectHostBase::setName(const QString &name)
{
    Q_D(QRemoteObjectHostBase);
    setObjectName(name);
    if (d->remoteObjectIo)
        d->remoteObjectIo->setObjectName(name);
}

/*!
    \internal The HostBase version of this method is protected so the method
    isn't exposed on RegistryHost nodes.
*/
QUrl QRemoteObjectHostBase::hostUrl() const
{
    Q_D(const QRemoteObjectHostBase);
    return d->remoteObjectIo->serverAddress();
}

/*!
    \internal The HostBase version of this method is protected so the method
    isn't exposed on RegistryHost nodes.
*/
bool QRemoteObjectHostBase::setHostUrl(const QUrl &hostAddress)
{
    Q_D(QRemoteObjectHostBase);
    if (d->remoteObjectIo) {
        d->m_lastError = ServerAlreadyCreated;
        return false;
    }
    else if (d->isInitialized.loadAcquire()) {
        d->m_lastError = RegistryAlreadyHosted;
        return false;
    }

    d->remoteObjectIo = new QRemoteObjectSourceIo(hostAddress, this);
    if (d->remoteObjectIo->m_server.isNull()) { //Invalid url/scheme
        d->m_lastError = HostUrlInvalid;
        delete d->remoteObjectIo;
        d->remoteObjectIo = 0;
        return false;
    }

    //If we've given a name to the node, set it on the sourceIo as well
    if (!objectName().isEmpty())
        d->remoteObjectIo->setObjectName(objectName());
    //Since we don't know whether setHostUrl or setRegistryUrl/setRegistryHost will be called first,
    //break it into two pieces.  setHostUrl connects the RemoteObjectSourceIo->[add/remove]RemoteObjectSource to QRemoteObjectReplicaNode->[add/remove]RemoteObjectSource
    //setRegistry* calls appropriately connect RemoteObjecSourcetIo->[add/remove]RemoteObjectSource to the registry when it is created
    QObject::connect(d->remoteObjectIo, SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), this, SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)));
    QObject::connect(d->remoteObjectIo, SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), this, SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)));

    return true;
}

/*!
    Returns the host address for the QRemoteObjectNode as a QUrl. If the Node
    is not a Host node, it return an empty QUrl.

    \sa setHostUrl()
*/
QUrl QRemoteObjectHost::hostUrl() const
{
    return QRemoteObjectHostBase::hostUrl();
}

/*!
    Sets the \a hostAddress for a host QRemoteObjectNode.

    Returns \c true if the Host address is set, otherwise \c false.
*/
bool QRemoteObjectHost::setHostUrl(const QUrl &hostAddress)
{
    return QRemoteObjectHostBase::setHostUrl(hostAddress);
}

bool QRemoteObjectRegistryHost::setRegistryUrl(const QUrl &registryUrl)
{
    Q_D(QRemoteObjectRegistryHost);
    if (setHostUrl(registryUrl)) {
        if (!d->remoteObjectIo) {
            d->m_lastError = ServerAlreadyCreated;
            return false;
        } else if (d->isInitialized.loadAcquire() || d->registry) {
            d->m_lastError = RegistryAlreadyHosted;
            return false;
        }

        QRegistrySource *remoteObject = new QRegistrySource(this);
        enableRemoting(remoteObject);
        d->registryAddress = d->remoteObjectIo->serverAddress();
        d->registrySource = remoteObject;
        //Connect RemoteObjectSourceIo->remoteObject[Added/Removde] to the registry Slot
        QObject::connect(this, SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), d->registrySource, SLOT(addSource(QRemoteObjectSourceLocation)));
        QObject::connect(this, SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), d->registrySource, SLOT(removeSource(QRemoteObjectSourceLocation)));
        QObject::connect(d->remoteObjectIo, SIGNAL(serverRemoved(QUrl)),d->registrySource, SLOT(removeServer(QUrl)));
        //onAdd/Remove update the known remoteObjects list in the RegistrySource, so no need to connect to the RegistrySource remoteObjectAdded/Removed signals
        d->setRegistry(acquire<QRemoteObjectRegistry>());
        return true;
    }
    return false;
}

/*!
    Returns the last error set.
*/
QRemoteObjectNode::ErrorCode QRemoteObjectNode::lastError() const
{
    Q_D(const QRemoteObjectNode);
    return d->m_lastError;
}

/*!
    Returns the address of the registry as a QUrl.  This will be an empty QUrl if there is not registry in use.

    \sa setRegistryUrl(), registry()
*/
QUrl QRemoteObjectNode::registryUrl() const
{
    Q_D(const QRemoteObjectNode);
    return d->registryAddress;
}

/*!
    Sets the \a registryAddress of the registry for a QRemoteObjectNode.

    Returns \c true if the Registry is set and initialized, otherwise it
    returns \c false. This call blocks waiting for the Registry to initialize.

    \sa registryUrl()
*/
bool QRemoteObjectNode::setRegistryUrl(const QUrl &registryAddress)
{
    Q_D(QRemoteObjectNode);
    if (d->isInitialized.loadAcquire() || d->registry) {
        d->m_lastError = RegistryAlreadyHosted;
        return false;
    }

    if (!connectToNode(registryAddress)) {
        d->m_lastError = RegistryNotAcquired;
        return false;
    }

    d->registryAddress = registryAddress;
    d->setRegistry(acquire<QRemoteObjectRegistry>());
    //Connect remoteObject[Added/Removed] to the registry Slot
    QObject::connect(this, SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), d->registry, SLOT(addSource(QRemoteObjectSourceLocation)));
    QObject::connect(this, SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), d->registry, SLOT(removeSource(QRemoteObjectSourceLocation)));
    return true;
}

void QRemoteObjectNodePrivate::setRegistry(QRemoteObjectRegistry *reg)
{
    Q_Q(QRemoteObjectNode);
    registry = reg;
    reg->setParent(q);
    //Make sure when we get the registry initialized, we update our replicas
    QObject::connect(reg, SIGNAL(initialized()), q, SLOT(onRegistryInitialized()));
    //Make sure we handle new RemoteObjectSources on Registry...
    QObject::connect(reg, SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), q, SLOT(onRemoteObjectSourceAdded(QRemoteObjectSourceLocation)));
    QObject::connect(reg, SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), q, SLOT(onRemoteObjectSourceRemoved(QRemoteObjectSourceLocation)));
}

/*!
    Blocks until this Node's \l Registry is initialized or \a timeout (in
    milliseconds) expires. Returns \c true if the \l Registry is successfully
    initialized upon return, or \c false otherwise.
*/
bool QRemoteObjectNode::waitForRegistry(int timeout)
{
    Q_D(QRemoteObjectNode);
    return d->registry->waitForSource(timeout);
}

/*!
    Connects a client node to the host node at \a address.

    Connections will remain valid until the host node is deleted or no longer
    accessible over a network.

    Once a client is connected to a host, valid Replicas can then be acquired
    if the corresponding Source is being remoted.

    Return \a true on success, \a false otherwise (usually an unrecognized url,
    or connecting to already connected address).
*/
bool QRemoteObjectNode::connectToNode(const QUrl &address)
{
    Q_D(QRemoteObjectNode);
    return d->initConnection(address);
}

/*!
    \keyword dynamic acquire
    Returns a QRemoteObjectDynamicReplica of the Source \a name.
*/
QRemoteObjectDynamicReplica *QRemoteObjectNode::acquire(const QString &name)
{
    return new QRemoteObjectDynamicReplica(this, name);
}

/*!
    Enables a host node to dynamically provide remote access to the QObject \a
    object. Client nodes connected to the node
    hosting this object may obtain Replicas of this Source.

    The \a name defines the lookup-name under which the QObject can be acquired
    using \a QRemoteObjectNode::acquire() . If not explicitly set then the name
    given in the QCLASSINFO_REMOTEOBJECT_TYPE will be used. If no such macro
    was defined for the QObject then the \a QObject::objectName() is used.

    Returns \c false if the current node is a client node, or if the QObject is already
    registered to be remoted, and \c true if remoting is successfully enabled
    for the dynamic QObject.

    \sa disableRemoting()
*/
bool QRemoteObjectHostBase::enableRemoting(QObject *object, const QString &name)
{
    Q_D(QRemoteObjectHostBase);
    if (!d->remoteObjectIo) {
        d->m_lastError = OperationNotValidOnClientNode;
        return false;
    }

    const QMetaObject *meta = object->metaObject();
    QString _name = name;
    const int ind = meta->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE);
    if (ind != -1) { //We have an object created from repc or at least with QCLASSINFO defined
        if (_name.isEmpty())
            _name = QString::fromLatin1(meta->classInfo(ind).value());
        while (true) {
            Q_ASSERT(meta->superClass()); //This recurses to QObject, which doesn't have QCLASSINFO_REMOTEOBJECT_TYPE
            if (ind != meta->superClass()->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE)) //At the point we don't find the same QCLASSINFO_REMOTEOBJECT_TYPE,
                            //we have the metaobject we should work from
                break;
            meta = meta->superClass();
        }
    } else { //This is a passed in QObject, use its API
        if (_name.isEmpty()) {
            _name = object->objectName();
            if (_name.isEmpty()) {
                d->m_lastError = MissingObjectName;
                qCWarning(QT_REMOTEOBJECT) << qPrintable(objectName()) << "enableRemoting() Error: Unable to Replicate an object that does not have objectName() set.";
                return false;
            }
        }
    }
    return d->remoteObjectIo->enableRemoting(object, meta, _name);
}

/*!
    This overload of enableRemoting() is specific to \l QAbstractItemModel types
    (or any type derived from \l QAbstractItemModel). This is useful if you want
    to have a model and the HMI for the model in different processes.

    The three required parameters are the \a model itself, the \a name by which
    to lookup the model, and the \a roles that should be exposed on the Replica
    side. If you want to synchronize selection between \l Source and \l
    Replica, the optional \a selectionModel parameter can be used. This is only
    recommended when using a single Replica.

    Behind the scenes, Qt Remote Objects batches data() lookups and prefetches
    data when possible to make the model interaction as responsive as possible.

    Returns \c false if the current node is a client node, or if the QObject is already
    registered to be remoted, and \c true if remoting is successfully enabled
    for the QAbstractItemModel.

    \sa disableRemoting()
 */
bool QRemoteObjectHostBase::enableRemoting(QAbstractItemModel *model, const QString &name, const QVector<int> roles, QItemSelectionModel *selectionModel)
{
    //This looks complicated, but hopefully there is a way to have an adapter be a template
    //parameter and this makes sure that is supported.
    QObject *adapter = QAbstractItemSourceAdapter::staticMetaObject.newInstance(Q_ARG(QAbstractItemModel*, model),
                                                                                Q_ARG(QItemSelectionModel*, selectionModel),
                                                                                Q_ARG(QVector<int>, roles));
    QAbstractItemAdapterSourceAPI<QAbstractItemModel, QAbstractItemSourceAdapter> *api =
        new QAbstractItemAdapterSourceAPI<QAbstractItemModel, QAbstractItemSourceAdapter>(name);
    return enableRemoting(model, api, adapter);
}

/*!
    \fn bool QRemoteObjectNode::enableRemoting(ObjectType *object)

    This templated function overload enables a host node to provide remote
    access to a QObject \a object with a specified (and compile-time checked)
    interface. Client nodes connected to the node hosting this object may
    obtain Replicas of this Source.

    This is best illustrated by example:
    \snippet doc_src_remoteobjects.cpp api_source

    Here the MinuteTimerSourceAPI is the set of Signals/Slots/Properties
    defined by the TimeModel.rep file. Compile time checks are made to verify
    the input QObject can expose the requested API, it will fail to compile
    otherwise. This allows a subset of \a object 's interface to be exposed,
    and allows the types of conversions supported by Signal/Slot connections.

    Returns \c false if the current node is a client node, or if the QObject is
    already registered to be remoted, and \c true if remoting is successfully
    enabled for the QObject.

    \sa disableRemoting()
*/

/*!
    \internal Enables a host node to provide remote access to a QObject \a
    object with the API defined by \a api. Client nodes connected to the node
    hosting this object may obtain Replicas of this Source.

    Returns \c false if the current node is a client node, or if the QObject is
    already registered to be remoted, and \c true if remoting is successfully
    enabled for the QObject.

    \sa disableRemoting()
*/
bool QRemoteObjectHostBase::enableRemoting(QObject *object, const SourceApiMap *api, QObject *adapter)
{
    Q_D(QRemoteObjectHostBase);
    return d->remoteObjectIo->enableRemoting(object, api, adapter);
}

/*!
    Disables remote access for the QObject \a remoteObject. Returns \c false if
    the current node is a client node or if the \a remoteObject is not
    registered, and returns \c true if remoting is successfully disabled for
    the Source object.

    \warning Replicas of this object will no longer be valid after calling this method.

    \sa enableRemoting()
*/
bool QRemoteObjectHostBase::disableRemoting(QObject *remoteObject)
{
    Q_D(QRemoteObjectHostBase);
    if (!d->remoteObjectIo) {
        d->m_lastError = OperationNotValidOnClientNode;
        return false;
    }

    if (!d->remoteObjectIo->disableRemoting(remoteObject)) {
        d->m_lastError = SourceNotRegistered;
        return false;
    }

    return true;
}

/*!
 Returns a pointer to a Replica which is specifically derived from \l
 QAbstractItemModel. The \a name provided must match the name used with the
 matching \l enableRemoting that put the Model on the network. The returned \c
 model will be empty until it is initialized with the \l Source.
 */
QAbstractItemReplica *QRemoteObjectNode::acquireModel(const QString &name)
{
    QAbstractItemReplicaPrivate *rep = acquire<QAbstractItemReplicaPrivate>(name);
    return new QAbstractItemReplica(rep);
}

QRemoteObjectHostBasePrivate::QRemoteObjectHostBasePrivate()
    : QRemoteObjectNodePrivate()
    , remoteObjectIo(Q_NULLPTR)
{ }

QRemoteObjectHostPrivate::QRemoteObjectHostPrivate()
    : QRemoteObjectHostBasePrivate()
{ }

QRemoteObjectRegistryHostPrivate::QRemoteObjectRegistryHostPrivate()
    : QRemoteObjectHostBasePrivate()
    , registrySource(Q_NULLPTR)
{ }

QT_END_NAMESPACE

#include "moc_qremoteobjectnode.cpp"
