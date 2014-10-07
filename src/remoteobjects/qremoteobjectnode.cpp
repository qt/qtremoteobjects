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
#include "qregistrysource_p.h"
#include "qremoteobjectreplica_p.h"
#include "qremoteobjectsource_p.h"

#include <QMetaProperty>

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

    qCDebug(QT_REMOTEOBJECT) << "timerEvent" << pendingReconnect.size();
}

QRemoteObjectReplica *QRemoteObjectNodePrivate::acquire(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name)
{
    qCDebug(QT_REMOTEOBJECT) << "Starting acquire for" << name;
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
        QMap<QString, QRemoteObjectSourcePrivate*>::const_iterator mapIt;
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
        qCDebug(QT_REMOTEOBJECT) << "Acquire - Created new instance" << name<<remoteObjectAddresses();
    }
    return instance;
}

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

        qCDebug(QT_REMOTEOBJECT) << idx << mm.name();
        if (mm.methodType() != QMetaMethod::Signal)
            continue;

        // try to connect to a signal on the parent that has the same method signature
        QByteArray sig = QMetaObject::normalizedSignature(mm.methodSignature().constData());
        qCDebug(QT_REMOTEOBJECT) << sig;
        if (them->indexOfSignal(sig.constData()) == -1)
            continue;

        sig.prepend(QSIGNAL_CODE + '0');
        const char * const csig = sig.constData();
        const bool res = QObject::connect(object, csig, instance, csig);
        Q_UNUSED(res);
        ++nConnections;

        qCDebug(QT_REMOTEOBJECT) << sig << res;
    }

    qCDebug(QT_REMOTEOBJECT) << "# connections =" << nConnections;
}

void QRemoteObjectNodePrivate::openConnectionIfNeeded(const QString &name)
{
    qCDebug(QT_REMOTEOBJECT) << Q_FUNC_INFO << name << this;
    if (remoteObjectAddresses().contains(name)) {
        initConnection(remoteObjectAddresses()[name]);
        qCDebug(QT_REMOTEOBJECT) << "openedConnection" << remoteObjectAddresses()[name];
    }
}

void QRemoteObjectNodePrivate::initConnection(const QUrl &address)
{
    if (requestedUrls.contains(address)) {
        qCWarning(QT_REMOTEOBJECT) << "Connection already initialized for " << address.toString();
        return;
    }

    requestedUrls.insert(address);

    ClientIoDevice *connection = m_factory.createDevice(address, this);
    Q_ASSERT_X(connection, Q_FUNC_INFO, "Could not create IODevice for client");

    knownNodes.insert(connection);
    qCDebug(QT_REMOTEOBJECT) << "Replica Connection isValid" << connection->isOpen();
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
    qCDebug(QT_REMOTEOBJECT) <<"onRemoteObjectSourceAdded"<< entry << replicas<<replicas.contains(entry.first);
    if (!entry.first.isEmpty()) {
        QRemoteObjectSourceLocations locs = registry->propAsVariant(0).value<QRemoteObjectSourceLocations>();
        locs[entry.first] = entry.second;
        //TODO Is there a way to extend QRemoteObjectSourceLocations in place?
        registry->setProperty(0, QVariant::fromValue(locs));
        qCDebug(QT_REMOTEOBJECT) << "onRemoteObjectSourceAdded, now locations =" << registry->propAsVariant(0).value<QRemoteObjectSourceLocations>()<<locs;
    }
    if (replicas.contains(entry.first)) //We have a replica waiting on this remoteObject
    {
        QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(entry.first).toStrongRef();
        if (!rep) { //replica has been deleted, remove from list
            replicas.remove(entry.first);
            return;
        }

        initConnection(entry.second);

        qCDebug(QT_REMOTEOBJECT) << "Called initConnection due to new RemoteObjectSource added via registry" << entry.first;
    }
}

void QRemoteObjectNodePrivate::onRemoteObjectSourceRemoved(const QRemoteObjectSourceLocation &entry)
{
    if (!entry.first.isEmpty()) {
        QRemoteObjectSourceLocations locs = registry->propAsVariant(0).value<QRemoteObjectSourceLocations>();
        locs.remove(entry.first);
        registry->setProperty(0, QVariant::fromValue(locs));
    }
}

void QRemoteObjectNodePrivate::onRegistryInitialized()
{
    qCDebug(QT_REMOTEOBJECT) << "Registry Initialized" << remoteObjectAddresses();

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
        qCDebug(QT_REMOTEOBJECT) << "Starting reconnect timer";
    }
}

void QRemoteObjectNodePrivate::onClientRead(QObject *obj)
{
    ClientIoDevice *connection = qobject_cast<ClientIoDevice*>(obj);
    Q_ASSERT(connection);

    if (!connection->read())
        return;

    using namespace QRemoteObjectPackets;

    const QRemoteObjectPacket* packet = connection->packet();
    switch (packet->id) {
    case QRemoteObjectPacket::ObjectList:
    {
        const QObjectListPacket *p = static_cast<const QObjectListPacket *>(packet);
        const QSet<QString> newObjects = p->objects.toSet();
        qCDebug(QT_REMOTEOBJECT) << "newObjects:" << newObjects;
        Q_FOREACH (const QString &remoteObject, newObjects) {
            qCDebug(QT_REMOTEOBJECT) << "  connectedSources.contains("<<remoteObject<<")"<<connectedSources.contains(remoteObject)<<replicas.contains(remoteObject);
            if (!connectedSources.contains(remoteObject)) {
                connectedSources[remoteObject] = connection;
                connection->addSource(remoteObject);
                if (replicas.contains(remoteObject)) //We have a replica waiting on this remoteObject
                {
                    QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(remoteObject).toStrongRef();
                    QConnectedReplicaPrivate *cRep = static_cast<QConnectedReplicaPrivate*>(rep.data());
                    if (rep && cRep->connectionToSource.isNull())
                    {
                        qCDebug(QT_REMOTEOBJECT) << "Test" << remoteObject<<replicas.keys();
                        qCDebug(QT_REMOTEOBJECT) << cRep;
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
    case QRemoteObjectPacket::InitPacket:
    {
        const QInitPacket *p = static_cast<const QInitPacket *>(packet);
        const QString object = p->name;
        qCDebug(QT_REMOTEOBJECT) << "InitObject-->" <<object << this;
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
    case QRemoteObjectPacket::InitDynamicPacket:
    {
        const QInitDynamicPacket *p = static_cast<const QInitDynamicPacket *>(packet);
        const QString object = p->name;
        qCDebug(QT_REMOTEOBJECT) << "InitObject-->" <<object << this;
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
    case QRemoteObjectPacket::RemoveObject:
    {
        const QRemoveObjectPacket *p = static_cast<const QRemoveObjectPacket *>(packet);
        qCDebug(QT_REMOTEOBJECT) << "RemoveObject-->" << p->name << this;
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
    case QRemoteObjectPacket::PropertyChangePacket:
    {
        const QPropertyChangePacket *p = static_cast<const QPropertyChangePacket *>(packet);
        const QString object = p->name;
        qCDebug(QT_REMOTEOBJECT) << "PropertyChange-->" << p->name << QString::fromLatin1(p->propertyName.constData());
        QSharedPointer<QRemoteObjectReplicaPrivate> rep = replicas.value(object).toStrongRef();
        if (rep)
        {
            const int offset = rep->m_metaObject->propertyOffset();
            const int index = rep->m_metaObject->indexOfProperty(p->propertyName.constData())-offset;
            rep->setProperty(index, p->value);
        } else { //replica has been deleted, remove from list
            replicas.remove(p->name);
        }
        break;
    }
    case QRemoteObjectPacket::InvokePacket:
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
            qCDebug(QT_REMOTEOBJECT) << "Replica Invoke-->" << p->name << rep->m_metaObject->method(p->index+rep->m_methodOffset).name() << p->index << rep->m_methodOffset;
            QMetaObject::activate(rep.data(), rep->metaObject(), p->index+rep->m_methodOffset, param.data());
        } else { //replica has been deleted, remove from list
            replicas.remove(p->name);
        }
        break;
    }
    }

    if (connection->bytesAvailable()) //have bytes left over, so recurse
        onClientRead(connection);
}


QRemoteObjectNode::QRemoteObjectNode()
    : d_ptr(new QRemoteObjectNodePrivate)
{
    qRegisterMetaTypeStreamOperators<QVector<int> >();
}

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

QRemoteObjectNode::~QRemoteObjectNode()
{
}

QUrl QRemoteObjectNode::hostUrl() const
{
    if (d_ptr->remoteObjectIo.isNull())
        return QUrl();

    return d_ptr->remoteObjectIo->serverAddress();
}

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
    //Since we don't know whether setHostUrl or setRegistryUrl/setRegistryHost will be called first,
    //break it into two pieces.  setHostUrl connects the RemoteObjectSourceIo->[add/remove]RemoteObjectSource to QRemoteObjectReplicaNode->[add/remove]RemoteObjectSource
    //setRegistry* calls appropriately connect RemoteObjecSourcetIo->[add/remove]RemoteObjectSource to the registry when it is created
    QObject::connect(d_ptr->remoteObjectIo.data(), SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)), d_ptr.data(), SIGNAL(remoteObjectAdded(QRemoteObjectSourceLocation)));
    QObject::connect(d_ptr->remoteObjectIo.data(), SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)), d_ptr.data(), SIGNAL(remoteObjectRemoved(QRemoteObjectSourceLocation)));

    return true;
}

QRemoteObjectNode::ErrorCode QRemoteObjectNode::lastError() const
{
    return d_ptr->m_lastError;
}

QUrl QRemoteObjectNode::registryUrl() const
{
    return d_ptr->registryAddress;
}

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
    //TODO - what should happen if you register a RemoteObjectSource on the Registry node, but the RegistrySource isn't connected?
    //Possible to have a way to cache the values, so they can be sent when a connection is made
    //Or, return false on enableRemoting, with an error about not having the registry?
    //Or possible to get the list of RemoteObjectSources from remoteObjectIo, and send when the connection is made (use that as the cache)
    return d_ptr->registry->waitForSource();
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

QRemoteObjectNode QRemoteObjectNode::createHostNode(const QUrl &hostAddress)
{
    return QRemoteObjectNode(hostAddress, QUrl());
}

QRemoteObjectNode QRemoteObjectNode::createRegistryHostNode(const QUrl &hostAddress)
{
    return QRemoteObjectNode(hostAddress, hostAddress);
}

QRemoteObjectNode QRemoteObjectNode::createNodeConnectedToRegistry(const QUrl &registryAddress)
{
    return QRemoteObjectNode(QUrl(), registryAddress);
}

QRemoteObjectNode QRemoteObjectNode::createHostNodeConnectedToRegistry(const QUrl &hostAddress, const QUrl &registryAddress)
{
    if (hostAddress == registryAddress) { //Assume hosting registry is NOT intended
        QRemoteObjectNode node(hostAddress, QUrl());
        node.d_ptr->m_lastError = UnintendedRegistryHosting;
        return node;
    }

    return QRemoteObjectNode(hostAddress, registryAddress);
}

void QRemoteObjectNode::connect(const QUrl &address)
{
    d_ptr->initConnection(address);
}

QRemoteObjectDynamicReplica *QRemoteObjectNode::acquire(const QString &name)
{
    QRemoteObjectDynamicReplica *instance = new QRemoteObjectDynamicReplica;
    return static_cast<QRemoteObjectDynamicReplica*>(d_ptr->acquire(Q_NULLPTR, instance, name));
}

bool QRemoteObjectNode::enableRemoting(QRemoteObjectSource *remoteObject)
{
    if (d_ptr->remoteObjectIo.isNull()) {
        d_ptr->m_lastError = OperationNotValidOnClientNode;
        return false;
    }

    const int ind = remoteObject->metaObject()->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE);
    const QString name = QString::fromLatin1(remoteObject->metaObject()->classInfo(ind).value());

    d_ptr->isInitialized.storeRelease(1);

    return d_ptr->remoteObjectIo->enableRemoting(remoteObject, &QRemoteObjectSource::staticMetaObject, name);
}

bool QRemoteObjectNode::enableRemoting(QObject *object, const QMetaObject *_meta)
{
    if (d_ptr->remoteObjectIo.isNull()) {
        d_ptr->m_lastError = OperationNotValidOnClientNode;
        return false;
    }

    const QMetaObject *meta = _meta;
    QString name;
    if (!meta) { //If meta isn't provided, we need to search for an object that has RemoteObject CLASSINFO
        meta = object->metaObject();
        int ind = meta->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE);
        while (meta && ind == -1) {
            meta = meta->superClass();
            if (meta)
                ind = meta->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE);
        }
        if (meta) {
            name = QString::fromLatin1(meta->classInfo(ind).value());
            meta = meta->superClass(); //We want the parent of the class that has ClassInfo, since we want to forward
                                       //the object_type API
        } else {
            name = object->objectName();
            if (name.isEmpty()) {
                d_ptr->m_lastError = MissingObjectName;
                qCWarning(QT_REMOTEOBJECT) << "enableRemoting() Error: Unable to Replicate an object that does not have objectName() set.";
                return false;
            }
            meta = object->metaObject()->superClass();  //*Assume* we only want object's API forwarded
        }
    } else {
        name = object->objectName();
        if (name.isEmpty()) {
            d_ptr->m_lastError = MissingObjectName;
            qCWarning(QT_REMOTEOBJECT) << "enableRemoting() Error: Unable to Replicate an object that does not have objectName() set.";
            return false;
        }
        const QMetaObject *check = object->metaObject();
        if (check == meta) {
            qCWarning(QT_REMOTEOBJECT) << "enableRemoting() Error: The QMetaObject pointer provided is for object.  An ancestor of object should be used.";
            return false;
        }
        while (check && check != meta)
            check = check->superClass();
        if (!check) { //Oops, meta is not a superclass of object
            qCWarning(QT_REMOTEOBJECT) << "enableRemoting() Error: The QMetaObject must be an ancestor of object.";
            return false;
        }
    }

    return d_ptr->remoteObjectIo->enableRemoting(object, meta, name);
}

bool QRemoteObjectNode::disableRemoting(QObject *remoteObject)
{
    if (d_ptr->remoteObjectIo.isNull()) {
        d_ptr->m_lastError = OperationNotValidOnClientNode;
        return false;
    }

    QRemoteObjectSourcePrivate *pp = remoteObject->findChild<QRemoteObjectSourcePrivate *>(QString(), Qt::FindDirectChildrenOnly);
    if (!pp) //We must be calling unregister before register was called.
        return false;

    if (!d_ptr->remoteObjectIo->disableRemoting(pp)) {
        d_ptr->m_lastError = SourceNotRegistered;
        return false;
    }

    return true;
}

QRemoteObjectReplica *QRemoteObjectNode::acquire(const QMetaObject *replicaMeta, QRemoteObjectReplica *instance)
{
    return d_ptr->acquire(replicaMeta, instance, name(replicaMeta));
}

QT_END_NAMESPACE
