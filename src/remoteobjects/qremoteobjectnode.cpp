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
    : QObject(Q_NULLPTR)
    , retryInterval(250)
    , m_lastError(QRemoteObjectNode::NoError)
{
    connect(&clientRead, SIGNAL(mapped(QObject*)), this, SLOT(onClientRead(QObject*)));
}

QRemoteObjectNodePrivate::~QRemoteObjectNodePrivate()
{
    Q_FOREACH (ClientIoDevice *conn, knownNodes) {
        conn->close();
        conn->deleteLater();
    }
}

QRemoteObjectSourceLocations QRemoteObjectNodePrivate::remoteObjectAddresses() const
{
    if (!registrySource.isNull())
        return registrySource->sourceLocations();
    else if (!registry.isNull())
        return registry->sourceLocations();
    return QRemoteObjectSourceLocations();
}

void QRemoteObjectNodePrivate::timerEvent(QTimerEvent*)
{
    Q_FOREACH (ClientIoDevice *conn, pendingReconnect) {
        if (conn->isOpen())
            pendingReconnect.remove(conn);
        else
            conn->connectToServer();
    }

    if (pendingReconnect.isEmpty())
        reconnectTimer.stop();

    qRODebug(this) << "timerEvent" << pendingReconnect.size();
}

QRemoteObjectReplica *QRemoteObjectNodePrivate::acquire(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name)
{
    qRODebug(this) << "Starting acquire for" << name;
    isInitialized.storeRelease(1);
    openConnectionIfNeeded(name);
    QMutexLocker locker(&mutex);
    if (hasInstance(name)) {
        qCDebug(QT_REMOTEOBJECT)<<"Acquire - using existing instance";
        QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(name).toStrongRef();
        Q_ASSERT(rep);
        instance->d_ptr = rep;
        rep->configurePrivate(instance);
    } else {
        QMap<QString, QRemoteObjectSource*>::const_iterator mapIt;
        if (!remoteObjectIo.isNull() && map_contains(remoteObjectIo->m_remoteObjects, name, mapIt)) {
            QInProcessReplicaPrivate *rp = new QInProcessReplicaPrivate(name, meta);
            instance->d_ptr.reset(rp);
            rp->configurePrivate(instance);
            connectReplica(mapIt.value()->m_object, instance);
            rp->connectionToSource = mapIt.value();
        } else {
            QConnectedReplicaPrivate *rp = new QConnectedReplicaPrivate(name, meta);
            instance->d_ptr.reset(rp);
            rp->configurePrivate(instance);
            if (connectedSources.contains(name)) { //Either we have a peer connections, or existing connection via registry
                rp->setConnection(connectedSources[name]);
            } else if (remoteObjectAddresses().contains(name)) { //No existing connection, but we know we can connect via registry
                initConnection(remoteObjectAddresses()[name]); //This will try the connection, and if successful, the remoteObjects will be sent
                                                      //The link to the replica will be handled then
            }
        }
        instance->initialize();
        replicas.insert(name, instance->d_ptr.toWeakRef());
        qRODebug(this) << "Acquire - Created new instance" << name<<remoteObjectAddresses();
    }
    return instance;
}

/*!
    Returns a pointer to the Node's \l {QRemoteObjectRegistry}, if the Node is using the Registry feature; otherwise it returns 0.
*/
const QRemoteObjectRegistry *QRemoteObjectNode::registry() const
{
    return d_ptr->registry.data();
}

void QRemoteObjectNodePrivate::connectReplica(QObject *object, QRemoteObjectReplica *instance)
{
    int nConnections = 0;
    const QMetaObject *us = instance->metaObject();
    const QMetaObject *them = object->metaObject();

    static const int memberOffset = QRemoteObjectReplica::staticMetaObject.methodCount();
    for (int idx = memberOffset; idx < us->methodCount(); ++idx) {
        const QMetaMethod mm = us->method(idx);

        qRODebug(this) << idx << mm.name();
        if (mm.methodType() != QMetaMethod::Signal)
            continue;

        // try to connect to a signal on the parent that has the same method signature
        QByteArray sig = QMetaObject::normalizedSignature(mm.methodSignature().constData());
        qRODebug(this) << sig;
        if (them->indexOfSignal(sig.constData()) == -1)
            continue;

        sig.prepend(QSIGNAL_CODE + '0');
        const char * const csig = sig.constData();
        const bool res = QObject::connect(object, csig, instance, csig);
        Q_UNUSED(res);
        ++nConnections;

        qRODebug(this) << sig << res;
    }

    qRODebug(this) << "# connections =" << nConnections;
}

void QRemoteObjectNodePrivate::openConnectionIfNeeded(const QString &name)
{
    qRODebug(this) << Q_FUNC_INFO << name << this;
    if (remoteObjectAddresses().contains(name)) {
        initConnection(remoteObjectAddresses()[name]);
        qRODebug(this) << "openedConnection" << remoteObjectAddresses()[name];
    }
}

void QRemoteObjectNodePrivate::initConnection(const QUrl &address)
{
    if (requestedUrls.contains(address)) {
        qROWarning(this) << "Connection already initialized for " << address.toString();
        return;
    }

    requestedUrls.insert(address);

    ClientIoDevice *connection = m_factory.createDevice(address, this);
    Q_ASSERT_X(connection, Q_FUNC_INFO, "Could not create IODevice for client");

    knownNodes.insert(connection);
    qRODebug(this) << "Replica Connection isValid" << connection->isOpen();
    connect(connection, SIGNAL(shouldReconnect(ClientIoDevice*)), this, SLOT(onShouldReconnect(ClientIoDevice*)));
    connection->connectToServer();
    connect(connection, SIGNAL(readyRead()), &clientRead, SLOT(map()));
    clientRead.setMapping(connection, connection);
}

bool QRemoteObjectNodePrivate::hasInstance(const QString &name)
{
    if (!replicas.contains(name))
        return false;

    QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(name).toStrongRef();
    if (!rep) { //already deleted
        replicas.remove(name);
        return false;
    }

    return true;
}

void QRemoteObjectNodePrivate::onRemoteObjectSourceAdded(const QRemoteObjectSourceLocation &entry)
{
    qRODebug(this) << "onRemoteObjectSourceAdded" << entry << replicas << replicas.contains(entry.first);
    if (!entry.first.isEmpty()) {
        QRemoteObjectSourceLocations locs = registry->sourceLocations();
        locs[entry.first] = entry.second;
        //TODO Is there a way to extend QRemoteObjectSourceLocations in place?
        registry->setProperty(0, QVariant::fromValue(locs));
        qRODebug(this) << "onRemoteObjectSourceAdded, now locations =" << locs;
    }
    if (replicas.contains(entry.first)) //We have a replica waiting on this remoteObject
    {
        QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(entry.first).toStrongRef();
        if (!rep) { //replica has been deleted, remove from list
            replicas.remove(entry.first);
            return;
        }

        initConnection(entry.second);

        qRODebug(this) << "Called initConnection due to new RemoteObjectSource added via registry" << entry.first;
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
    qRODebug(this) << "Registry Initialized" << remoteObjectAddresses();

    QHashIterator<QString, QUrl> i(remoteObjectAddresses());
    while (i.hasNext()) {
        i.next();
        if (replicas.contains(i.key())) //We have a replica waiting on this remoteObject
        {
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(i.key()).toStrongRef();
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
    pendingReconnect.insert(ioDevice);

    Q_FOREACH (const QString &remoteObject, ioDevice->remoteObjects()) {
        connectedSources.remove(remoteObject);
        ioDevice->removeSource(remoteObject);
        if (replicas.contains(remoteObject)) { //We have a replica waiting on this remoteObject
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(remoteObject).toStrongRef();
            QConnectedReplicaPrivate *cRep = static_cast<QConnectedReplicaPrivate*>(rep.data());
            if (rep && !cRep->connectionToSource.isNull()) {
                cRep->setDisconnected();
            } else if (!rep) {
                replicas.remove(remoteObject);
            }
        }
    }
    if (!reconnectTimer.isActive()) {
        reconnectTimer.start(retryInterval, this);
        qRODebug(this) << "Starting reconnect timer";
    }
}

void QRemoteObjectNodePrivate::onClientRead(QObject *obj)
{
    ClientIoDevice *connection = qobject_cast<ClientIoDevice*>(obj);
    Q_ASSERT(connection);

    do {

        if (!connection->read())
            return;

        using namespace QRemoteObjectPackets;

        const QRemoteObjectPacket* packet = connection->packet();
        switch (packet->id) {
        case ObjectList:
        {
            const QObjectListPacket *p = static_cast<const QObjectListPacket *>(packet);
            const QSet<QString> newObjects = p->objects.toSet();
            qRODebug(this) << "newObjects:" << newObjects;
            Q_FOREACH (const QString &remoteObject, newObjects) {
                qRODebug(this) << "  connectedSources.contains("<<remoteObject<<")"<<connectedSources.contains(remoteObject)<<replicas.contains(remoteObject);
                if (!connectedSources.contains(remoteObject)) {
                    connectedSources[remoteObject] = connection;
                    connection->addSource(remoteObject);
                    if (replicas.contains(remoteObject)) //We have a replica waiting on this remoteObject
                    {
                        QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(remoteObject).toStrongRef();
                        QConnectedReplicaPrivate *cRep = static_cast<QConnectedReplicaPrivate*>(rep.data());
                        if (rep && cRep->connectionToSource.isNull())
                        {
                            qRODebug(this) << "Test" << remoteObject<<replicas.keys();
                            qRODebug(this) << cRep;
                            cRep->setConnection(connection);
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
            const QInitPacket *p = static_cast<const QInitPacket *>(packet);
            const QString object = p->name;
            qRODebug(this) << "InitObject-->" <<object << this;
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(object).toStrongRef();
            if (rep)
            {
                QConnectedReplicaPrivate *cRep = static_cast<QConnectedReplicaPrivate*>(rep.data());
                cRep->initialize(p->packetData);
            } else { //replica has been deleted, remove from list
                replicas.remove(object);
            }
            break;
        }
        case InitDynamicPacket:
        {
            const QInitDynamicPacket *p = static_cast<const QInitDynamicPacket *>(packet);
            const QString object = p->name;
            qRODebug(this) << "InitObject-->" <<object << this;
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(object).toStrongRef();
            if (rep)
            {
                QConnectedReplicaPrivate *cRep = static_cast<QConnectedReplicaPrivate*>(rep.data());
                cRep->initializeMetaObject(p);

            } else { //replica has been deleted, remove from list
                replicas.remove(object);
            }
            break;
        }
        case RemoveObject:
        {
            const QRemoveObjectPacket *p = static_cast<const QRemoveObjectPacket *>(packet);
            qRODebug(this) << "RemoveObject-->" << p->name << this;
            connectedSources.remove(p->name);
            connection->removeSource(p->name);
            if (replicas.contains(p->name)) { //We have a replica waiting on this remoteObject
                QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(p->name).toStrongRef();
                QConnectedReplicaPrivate *cRep = static_cast<QConnectedReplicaPrivate*>(rep.data());
                if (rep && !cRep->connectionToSource.isNull()) {
                    cRep->connectionToSource.clear();
                    if (cRep->isReplicaValid()) {
                        //Changed from receiving to not receiving
                        cRep->emitValidChanged();
                    }
                } else if (!rep) {
                    replicas.remove(p->name);
                }
            }
            break;
        }
        case PropertyChangePacket:
        {
            const QPropertyChangePacket *p = static_cast<const QPropertyChangePacket *>(packet);
            const QString object = p->name;
            qRODebug(this) << "PropertyChange-->" << p->name << QString::fromLatin1(p->propertyName.string);
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(object).toStrongRef();
            if (rep)
            {
                const int offset = rep->m_metaObject->propertyOffset();
                const int index = rep->m_metaObject->indexOfProperty(p->propertyName.string)-offset;
                rep->setProperty(index, p->value);
            } else { //replica has been deleted, remove from list
                replicas.remove(p->name);
            }
            break;
        }
        case InvokePacket:
        {
            const QInvokePacket *p = static_cast<const QInvokePacket *>(packet);
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(p->name).toStrongRef();
            if (rep)
            {
                static QVariant null(QMetaType::QObjectStar, (void*)0);

                // Qt usually supports 9 arguments, so ten should be usually safe
                QVarLengthArray<void*, 10> param(p->args.size() + 1);
                param[0] = null.data(); //Never a return value
                for (int i = 0; i < p->args.size(); i++) {
                    param[i + 1] = const_cast<void *>(p->args[i].data());
                }
                qRODebug(this) << "Replica Invoke-->" << p->name << rep->m_metaObject->method(p->index+rep->m_signalOffset).name() << p->index << rep->m_signalOffset;
                QMetaObject::activate(rep.data(), rep->metaObject(), p->index+rep->m_signalOffset, param.data());
            } else { //replica has been deleted, remove from list
                replicas.remove(p->name);
            }
            break;
        }
        case InvokeReplyPacket:
        {
            const QInvokeReplyPacket *p = static_cast<const QInvokeReplyPacket *>(packet);
            QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(p->name).toStrongRef();
            if (rep) {
                qRODebug(this) << "Received InvokeReplyPacket ack'ing serial id:" << p->ackedSerialId;
                rep->notifyAboutReply(p);
            } else { //replica has been deleted, remove from list
                replicas.remove(p->name);
            }
            break;
        }
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

/*!
    Default constructor for QRemoteObjectNode. A Node constructed in this
    manner can not be connected to, and thus can not expose Source objects on
    the network. It also will not include a QRemoteObjectRegistry.

    \sa createHostNode(), createRegistryHostNode(), createNodeConnectedToRegistry(), createHostNodeConnectedToRegistry()
*/
QRemoteObjectNode::QRemoteObjectNode()
    : d_ptr(new QRemoteObjectNodePrivate)
{
    qRegisterMetaTypeStreamOperators<QVector<int> >();
}

/*!
    \internal Constructs a new host QRemoteObjectNode at \a hostAddress and
    connected to the registry at \a registryAddress. If the given registry
    address and host address are equal, this node will host the registry as
    well.
*/
QRemoteObjectNode::QRemoteObjectNode(const QUrl &hostAddress, const QUrl &registryAddress)
    : d_ptr(new QRemoteObjectNodePrivate)
{
    qRegisterMetaTypeStreamOperators<QVector<int> >();
    if (!hostAddress.isEmpty()) {
        setHostUrl(hostAddress);
        if (hostAddress == registryAddress) {
            hostRegistry();
            return;
        }
    }

    if (!registryAddress.isEmpty())
        setRegistryUrl(registryAddress);
}

/*!
    Destructor for QRemoteObjectNode.
*/
QRemoteObjectNode::~QRemoteObjectNode()
{
}

/*!
    Sets \a name as the internal name for this Node.  This
    is then output as part of the logging (if enabled).
    This is primarily useful if you merge log data from multiple nodes.
*/
void QRemoteObjectNode::setName(const QString &name)
{
    d_ptr->setObjectName(name);
    if (d_ptr->remoteObjectIo)
        d_ptr->remoteObjectIo->setObjectName(name);
}

/*!
    Returns the host address for the QRemoteObjectNode as a QUrl. If the Node
    is not a Host node, it return an empty QUrl.

    \sa setHostUrl()
*/
QUrl QRemoteObjectNode::hostUrl() const
{
    if (d_ptr->remoteObjectIo.isNull())
        return QUrl();

    return d_ptr->remoteObjectIo->serverAddress();
}

/*!
    Sets the \a hostAddress for a host QRemoteObjectNode. It is recommended
    that the static constructors for a Node be used to establish a Host Node
    during Node construction.

    Returns \c true if the Host address is set, otherwise \c false.

    \sa createHostNode(), createRegistryHostNode(), createHostNodeConnectedToRegistry()
*/
bool QRemoteObjectNode::setHostUrl(const QUrl &hostAddress)
{
    if (!d_ptr->remoteObjectIo.isNull()) {
        d_ptr->m_lastError = ServerAlreadyCreated;
        return false;
    }
    else if (d_ptr->isInitialized.loadAcquire()) {
        d_ptr->m_lastError = RegistryAlreadyHosted;
        return false;
    }

    d_ptr->remoteObjectIo.reset(new QRemoteObjectSourceIo(hostAddress));
    //If we've given a name to the node, set it on the sourceIo as well
    if (!d_ptr->objectName().isEmpty())
        d_ptr->remoteObjectIo->setObjectName(d_ptr->objectName());
    //Since we don't know whether setHostUrl or setRegistryUrl/setRegistryHost will be called first,
    //break it into two pieces.  setHostUrl connects the RemoteObjectSourceIo->[add/remove]RemoteObjectSource to QRemoteObjectReplicaNode->[add/remove]RemoteObjectSource
    //setRegistry* calls appropriately connect RemoteObjecSourcetIo->[add/remove]RemoteObjectSource to the registry when it is created
    QObject::connect(d_ptr->remoteObjectIo.data(), SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), d_ptr.data(), SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)));
    QObject::connect(d_ptr->remoteObjectIo.data(), SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), d_ptr.data(), SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)));

    return true;
}

/*!
    Returns the last error set.
*/
QRemoteObjectNode::ErrorCode QRemoteObjectNode::lastError() const
{
    return d_ptr->m_lastError;
}

/*!
    Returns the address of the registry as a QUrl.  This will be an empty QUrl if there is not registry in use.

    \sa setRegistryUrl(), registry()
*/
QUrl QRemoteObjectNode::registryUrl() const
{
    return d_ptr->registryAddress;
}

/*!
    Sets the \a registryAddress of the registry for a QRemoteObjectNode. It is
    recommended that the static constructors for a Node be used to establish a
    Host Node during Node construction.

    Returns \c true if the Registry is set and initialized, otherwise it
    returns \c false. This call blocks waiting for the Registry to initialize.

    \sa registryUrl(), createRegistryHostNode(), createNodeConnectedToRegistry(), createHostNodeConnectedToRegistry()
*/
bool QRemoteObjectNode::setRegistryUrl(const QUrl &registryAddress)
{
    if (d_ptr->isInitialized.loadAcquire() || ! d_ptr->registry.isNull()) {
        d_ptr->m_lastError = RegistryAlreadyHosted;
        return false;
    }

    connect(registryAddress);
    d_ptr->registryAddress = registryAddress;
    d_ptr->setRegistry(acquire<QRemoteObjectRegistry>());
    //Connect remoteObject[Added/Removed] to the registry Slot
    QObject::connect(d_ptr.data(), SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), d_ptr->registry.data(), SLOT(addSource(QRemoteObjectSourceLocation)));
    QObject::connect(d_ptr.data(), SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), d_ptr->registry.data(), SLOT(removeSource(QRemoteObjectSourceLocation)));
    return true;
}

void QRemoteObjectNodePrivate::setRegistry(QRemoteObjectRegistry *reg)
{
    registry.reset(reg);
    //Make sure when we get the registry initialized, we update our replicas
    QObject::connect(reg, SIGNAL(initialized()), this, SLOT(onRegistryInitialized()));
    //Make sure we handle new RemoteObjectSources on Registry...
    QObject::connect(reg, SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), this, SLOT(onRemoteObjectSourceAdded(QRemoteObjectSourceLocation)));
    QObject::connect(reg, SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), this, SLOT(onRemoteObjectSourceRemoved(QRemoteObjectSourceLocation)));
}

/*!
    Sets the current QRemoteObjectNode to be the registry host. Returns \c
    false if the registry is already being hosted or if the node has not been
    initialized, and returns \c true if the registry is successfully set.
*/
bool QRemoteObjectNode::hostRegistry()
{
    if (d_ptr->remoteObjectIo.isNull()) {
        d_ptr->m_lastError = ServerAlreadyCreated;
        return false;
    }
    else if (d_ptr->isInitialized.loadAcquire() || !d_ptr->registry.isNull()) {
        d_ptr->m_lastError = RegistryAlreadyHosted;
        return false;
    }

    QRegistrySource *remoteObject = new QRegistrySource;
    enableRemoting(remoteObject);
    d_ptr->registryAddress = d_ptr->remoteObjectIo->serverAddress();
    d_ptr->registrySource.reset(remoteObject);
    //Connect RemoteObjectSourceIo->remoteObject[Added/Removde] to the registry Slot
    QObject::connect(d_ptr.data(), SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), d_ptr->registrySource.data(), SLOT(addSource(QRemoteObjectSourceLocation)));
    QObject::connect(d_ptr.data(), SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), d_ptr->registrySource.data(), SLOT(removeSource(QRemoteObjectSourceLocation)));
    QObject::connect(d_ptr->remoteObjectIo.data(), SIGNAL(serverRemoved(QUrl)),d_ptr->registrySource.data(), SLOT(removeServer(QUrl)));
    //onAdd/Remove update the known remoteObjects list in the RegistrySource, so no need to connect to the RegistrySource remoteObjectAdded/Removed signals
    d_ptr->setRegistry(acquire<QRemoteObjectRegistry>());
    return true;
}

/*!
    Blocks until this Node's \l Registry is initialized or \a timeout (in
    milliseconds) expires. Returns \c true if the \l Registry is successfully
    initialized upon return, or \c false otherwise.
*/
bool QRemoteObjectNode::waitForRegistry(int timeout)
{
    return d_ptr->registry->waitForSource(timeout);
}

/*!
    Constructs a new host QRemoteObjectNode with address \a hostAddress.

    \warning This node will not be connected to the registry. To create a host
    node connected to the registry, use createHostNodeConnectedToRegistry().
*/
QRemoteObjectNode QRemoteObjectNode::createHostNode(const QUrl &hostAddress)
{
    return QRemoteObjectNode(hostAddress, QUrl());
}

/*!
    Constructs a new host QRemoteObjectNode at \a hostAddress. The node will also host the registry.
*/
QRemoteObjectNode QRemoteObjectNode::createRegistryHostNode(const QUrl &hostAddress)
{
    return QRemoteObjectNode(hostAddress, hostAddress);
}

/*!
    Constructs a new client QRemoteObjectNode, connected to the registry located at \a registryAddress.
*/
QRemoteObjectNode QRemoteObjectNode::createNodeConnectedToRegistry(const QUrl &registryAddress)
{
    return QRemoteObjectNode(QUrl(), registryAddress);
}

/*!
    Constructs a new host QRemoteObjectNode at \a hostAddress, and connects it
    to the registry located at \a registryAddress.
*/
QRemoteObjectNode QRemoteObjectNode::createHostNodeConnectedToRegistry(const QUrl &hostAddress, const QUrl &registryAddress)
{
    if (hostAddress == registryAddress) { //Assume hosting registry is NOT intended
        QRemoteObjectNode node(hostAddress, QUrl());
        node.d_ptr->m_lastError = UnintendedRegistryHosting;
        return node;
    }

    return QRemoteObjectNode(hostAddress, registryAddress);
}

/*!
    Connects a client node to the host node at \a address.

    Connections will remain valid until the host node is deleted or no longer
    accessible over a network.

    Once a client is connected to a host, valid Replicas can then be acquired
    if the corresponding Source is being remoted.
*/
void QRemoteObjectNode::connect(const QUrl &address)
{
    d_ptr->initConnection(address);
}

/*!
    \keyword dynamic acquire
    Returns a QRemoteObjectDynamicReplica of the Source \a name.
*/
QRemoteObjectDynamicReplica *QRemoteObjectNode::acquire(const QString &name)
{
    QRemoteObjectDynamicReplica *instance = new QRemoteObjectDynamicReplica;
    return static_cast<QRemoteObjectDynamicReplica*>(d_ptr->acquire(Q_NULLPTR, instance, name));
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
bool QRemoteObjectNode::enableRemoting(QObject *object, const QString &name)
{
    if (d_ptr->remoteObjectIo.isNull()) {
        d_ptr->m_lastError = OperationNotValidOnClientNode;
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
                d_ptr->m_lastError = MissingObjectName;
                qCWarning(QT_REMOTEOBJECT) << qPrintable(d_ptr->objectName()) << "enableRemoting() Error: Unable to Replicate an object that does not have objectName() set.";
                return false;
            }
        }
    }
    return d_ptr->remoteObjectIo->enableRemoting(object, meta, _name);
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
bool QRemoteObjectNode::enableRemoting(QAbstractItemModel *model, const QString &name, const QVector<int> roles, QItemSelectionModel *selectionModel)
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
bool QRemoteObjectNode::enableRemoting(QObject *object, const SourceApiMap *api, QObject *adapter)
{
    return d_ptr->remoteObjectIo->enableRemoting(object, api, adapter);
}

/*!
    Disables remote access for the QObject \a remoteObject. Returns \c false if
    the current node is a client node or if the \a remoteObject is not
    registered, and returns \c true if remoting is successfully disabled for
    the Source object.

    \warning Replicas of this object will no longer be valid after calling this method.

    \sa enableRemoting()
*/
bool QRemoteObjectNode::disableRemoting(QObject *remoteObject)
{
    if (d_ptr->remoteObjectIo.isNull()) {
        d_ptr->m_lastError = OperationNotValidOnClientNode;
        return false;
    }

    if (!d_ptr->remoteObjectIo->disableRemoting(remoteObject)) {
        d_ptr->m_lastError = SourceNotRegistered;
        return false;
    }

    return true;
}

/*!
    \internal
    Returns a Replica of the Source sharing information in the meta-object \a replicaMeta and class \a instance.
*/
QRemoteObjectReplica *QRemoteObjectNode::acquire(const QMetaObject *replicaMeta, QRemoteObjectReplica *instance, const QString &name)
{
    return d_ptr->acquire(replicaMeta, instance, name.isEmpty() ? ::name(replicaMeta) : name);
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

QT_END_NAMESPACE
