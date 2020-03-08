/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
#include "qremoteobjectabstractitemmodelreplica_p.h"
#include "qremoteobjectabstractitemmodeladapter_p.h"
#include <QtCore/qabstractitemmodel.h>
#include <memory>

QT_BEGIN_NAMESPACE

using namespace QtRemoteObjects;
using namespace QRemoteObjectStringLiterals;

using GadgetType = QVector<QVariant>;
using RegisteredType = QPair<GadgetType, std::shared_ptr<QMetaObject>>;
static QMutex s_managedTypesMutex;
static QHash<int, RegisteredType> s_managedTypes;
static QHash<int, QSet<IoDeviceBase*>> s_trackedConnections;

static void GadgetsStaticMetacallFunction(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::ReadProperty) {
        GadgetType *_t = reinterpret_cast<GadgetType *>(_o);
        if (_id < _t->size()) {
            const auto &prop = _t->at(_id);
            QMetaType::destruct(int(prop.userType()), _a[0]);
            QMetaType::construct(int(prop.userType()), _a[0], prop.constData());
        }
    } else if (_c == QMetaObject::WriteProperty) {
        GadgetType *_t = reinterpret_cast<GadgetType *>(_o);
        if (_id < _t->size()) {
            auto & prop = (*_t)[_id];
            prop = QVariant(prop.userType(), _a[0]);
        }
    }
}

static void GadgetTypedDestructor(int, void *ptr)
{
    reinterpret_cast<GadgetType*>(ptr)->~GadgetType();
}

static void *GadgetTypedConstructor(int type, void *where, const void *copy)
{
    GadgetType *ret = where ? new(where) GadgetType : new GadgetType;
    if (copy) {
        *ret = *reinterpret_cast<const GadgetType*>(copy);
    } else {
        QMutexLocker lock(&s_managedTypesMutex);
        auto it = s_managedTypes.find(type);
        if (it == s_managedTypes.end()) {
            delete ret;
            return nullptr;
        }
        *ret = it->first;
    }
    return ret;
}

static void GadgetSaveOperator(QDataStream & out, const void *data)
{
    const GadgetType *gadgetProperties = reinterpret_cast<const GadgetType *>(data);
    for (const auto &prop : *gadgetProperties)
        out << prop;
}

static void GadgetLoadOperator(QDataStream &in, void *data)
{
    GadgetType *gadgetProperties = reinterpret_cast<GadgetType *>(data);
    for (auto &prop : *gadgetProperties)
        in >> prop;
}

// Like the Q_GADGET static methods above, we need constructor/destructor methods
// in order to use dynamically defined enums with QVariant or as signal/slot
// parameters (i.e., the queued connection mechanism, which QtRO leverages).
//
// We will need the enum methods to support different sizes when typed scope enum
// support is added, so might as well use that now.
template<typename T>
static void EnumDestructor(void *ptr)
{
    static_cast<T*>(ptr)->~T();
}

template<typename T>
static void *EnumConstructor(void *where, const void *copy)
{
    T *ret = where ? new(where) T : new T;
    if (copy)
        *ret = *static_cast<const T*>(copy);
    return ret;
}

// Not used, but keeping these in case we end up with a need for save/load.
template<typename T>
static void EnumSaveOperator(QDataStream & out, const void *data)
{
    const T value = *static_cast<const T *>(data);
    out << value;
}

template<typename T>
static void EnumLoadOperator(QDataStream &in, void *data)
{
    T value = *static_cast<T *>(data);
    in >> value;
}

static QString name(const QMetaObject * const mobj)
{
    const int ind = mobj->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE);
    return ind >= 0 ? QString::fromLatin1(mobj->classInfo(ind).value()) : QString();
}

QString QtRemoteObjects::getTypeNameAndMetaobjectFromClassInfo(const QMetaObject *& meta) {
    QString typeName;
    const int ind = meta->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE);
    if (ind != -1) { //We have an object created from repc or at least with QCLASSINFO defined
        typeName = QString::fromLatin1(meta->classInfo(ind).value());
        while (true) {
            Q_ASSERT(meta->superClass());//This recurses to QObject, which doesn't have QCLASSINFO_REMOTEOBJECT_TYPE
            //At the point superclass doesn't have the same QCLASSINFO_REMOTEOBJECT_TYPE,
            //we have the metaobject we should work from
            if (ind != meta->superClass()->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE))
                break;
            meta = meta->superClass();
        }
    }
    return typeName;
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

/*!
    \qmltype Node
    \instantiates QRemoteObjectNode
    \inqmlmodule QtRemoteObjects
    \brief A node on a Qt Remote Objects network.

    The Node type provides an entry point to a Qt Remote Objects network. A network
    can be as simple as two nodes, or an arbitrarily complex set of processes and devices.

    A Node does not have a url that other nodes can connect to, and thus is able to acquire
    replicas only. It is not able to share source objects.

*/

QRemoteObjectNodePrivate::QRemoteObjectNodePrivate()
    : QObjectPrivate()
    , registry(nullptr)
    , retryInterval(250)
    , lastError(QRemoteObjectNode::NoError)
    , persistedStore(nullptr)
{ }

QRemoteObjectNodePrivate::~QRemoteObjectNodePrivate()
{
}

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

/*!
    \reimp
*/
void QRemoteObjectNode::timerEvent(QTimerEvent*)
{
    Q_D(QRemoteObjectNode);

    for (auto it = d->pendingReconnect.begin(), end = d->pendingReconnect.end(); it != end; /*erasing*/) {
        const auto &conn = *it;
        if (conn->isOpen()) {
            it = d->pendingReconnect.erase(it);
        } else {
            conn->connectToServer();
            ++it;
        }
    }

    if (d->pendingReconnect.isEmpty())
        d->reconnectTimer.stop();

    qRODebug(this) << "timerEvent" << d->pendingReconnect.size();
}

/*!
    \qmlproperty int Node::heartbeatInterval

    Heartbeat interval in ms.

    The heartbeat (only helpful for socket connections) will periodically send a
    message to connected nodes to detect whether the connection was disrupted.
    Qt Remote Objects will try to reconnect automatically if it detects a dropped
    connection. This function can help with that detection since the client will
    only detect that the server is unavailable when it tries to send data.

    A value of \c 0 (the default) will disable the heartbeat.
*/


/*!
    \property QRemoteObjectNode::heartbeatInterval
    \brief Heartbeat interval in ms.

    The heartbeat (only helpful for socket connections) will periodically send a
    message to connected nodes to detect whether the connection was disrupted.
    Qt Remote Objects will try to reconnect automatically if it detects a dropped
    connection. This function can help with that detection since the client will
    only detect that the server is unavailable when it tries to send data.

    A value of \c 0 (the default) will disable the heartbeat.
*/
int QRemoteObjectNode::heartbeatInterval() const
{
    Q_D(const QRemoteObjectNode);
    return d->m_heartbeatInterval;
}

void QRemoteObjectNode::setHeartbeatInterval(int interval)
{
    Q_D(QRemoteObjectNode);
    if (d->m_heartbeatInterval == interval)
        return;
    d->m_heartbeatInterval = interval;
    emit heartbeatIntervalChanged(interval);
}

/*!
    \since 5.12
    \typedef QRemoteObjectNode::RemoteObjectSchemaHandler

    Typedef for a std::function method that can take a QUrl input and is
    responsible for creating the communications channel between this node and
    the node hosting the desired \l Source. As some types of QIODevices (e.g.,
    QSslSocket) require additional steps before the device is ready for use,
    the method is responsible for calling \l addClientSideConnection once the
    connection is fully established.
*/

/*!
    \since 5.12
    \brief Provide a custom method to handle externally provided schemas

    This method is tied to the \l Registry and \l {External Schemas}. By
    registering a std::function handler for an external schema, the registered
    method will be called when the registry is notified of a \l Source you've
    acquired being available. Without this registration, QtRO would only be
    able to handle the "built-in" schemas.

    The provided method, \a handler, will be called when the registry sees a \l
    Source object on a new (not yet connected) Node with a {QUrl::schema()} of
    \a schema. The \a handler, of type \l
    QRemoteObjectNode::RemoteObjectSchemaHandler will get the \l QUrl of the
    Node providing the \l Source as an input parameter, and is responsible for
    establishing the communications channel (a \l QIODevice of some sort) and
    calling \l addClientSideConnection with it.

    \sa RemoteObjectSchemaHandler
*/
void QRemoteObjectNode::registerExternalSchema(const QString &schema, QRemoteObjectNode::RemoteObjectSchemaHandler handler)
{
    Q_D(QRemoteObjectNode);
    d->schemaHandlers.insert(schema, handler);
}

/*!
    \since 5.11
    \brief Forward Remote Objects from another network

    The proxy functionality is useful when you want to share \l Source objects
    over multiple networks. For instance, if you have an embedded target using
    target-only connections (like local) and you want to make some of those
    same objects available externally.

    As a concrete example, say you have a set of processes talking to each
    other on your target hardware using a registry, with the \l Registry at
    "local:registry" and separate processes using a node at "local:MyHost" that
    holds \l Source objects. If you wanted to access these objects, but over
    tcp, you could create a new proxyNode like so:

\code
    // myInternalHost is a node only visible on the device...
    QRemoteObjectHost myInternalHost("local:MyHost");
    myInternalHost.enableRemoting<SomeObject>(&someObject);

    // Regular host node, listening on port 12123, so visible to other
    // devices
    QRemoteObjectHost proxyNode("tcp://localhost:12123");

    // Enable proxying objects from nodes on the local machine's internal
    // QtRO bus
    proxyNode.proxy("local:registry");
\endcode

    And from another device you create another node:

\code
    // NB: localhost resolves to a different ip address than proxyNode
    QRemoteObjectHost nodeOnRemoteDevice("tcp://localhost:23234");

    // Connect to the target's proxyNode directly, or use a tcp registry...
    nodeOnRemoteDevice.connectToNode("tcp://<target device>:12123");

    // Because of the proxy, we can get the object over tcp/ip port 12123,
    // even though we can't connect directly to "local:MyHost"
    SomeObject *so = nodeOnRemoteDevice.acquire<SomeObject>();
\endcode

    This would (internally) create a node in proxyNode, which (again
    internally/automatically) connects to the provided registry (given by the
    \a registryUrl parameter, "local:registry" in this example). Whenever
    local:registry emits the \l remoteObjectAdded signal, the
    \c QRemoteObjectSourceLocation is passed to the \a filter given to the proxy
    call. If this method returns true (the default filter simply returns true
    without any filtering), the object is acquired() from the internal node and
    enableRemoting() (once the replica is initialized) is called on proxyNode.

    If a \a hostUrl is provided (which is required to enable reverseProxy, but
    not needed otherwise), the internal node will be a \l QRemoteObjectHost node
    configured with the provided address. If no \a hostUrl is provided, the
    internal node will be a QRemoteObjectNode (not HostNode).

    Returns \c true if the object is acquired from the internal node.

    \sa reverseProxy()
*/
bool QRemoteObjectHostBase::proxy(const QUrl &registryUrl, const QUrl &hostUrl, RemoteObjectNameFilter filter)
{
    Q_D(QRemoteObjectHostBase);
    if (!registryUrl.isValid() || !QtROClientFactory::instance()->isValid(registryUrl)) {
        qROWarning(this) << "Can't proxy to registryUrl (invalid url or schema)" << registryUrl;
        return false;
    }

    if (!hostUrl.isEmpty() && !QtROClientFactory::instance()->isValid(hostUrl)) {
        qROWarning(this) << "Can't proxy using hostUrl (invalid schema)" << hostUrl;
        return false;
    }

    if (d->proxyInfo) {
        qROWarning(this) << "Proxying from multiple objects is currently not supported.";
        return false;
    }

    QRemoteObjectNode *node;
    if (hostUrl.isEmpty()) {
        node = new QRemoteObjectNode(registryUrl);
    } else {
        node = new QRemoteObjectHost(hostUrl, registryUrl);
    }
    d->proxyInfo = new ProxyInfo(node, this, filter);
    return true;
}

/*!
    \since 5.11
    \brief Forwards remote objects to another network.

    The reverseProxy() function allows the \l proxy() functionality to be
    extended, in effect mirroring the proxy functionality in the "reverse"
    direction. These are distinct, because node communication is not symmetric,
    one side calls enableRemoting() with a \l Source object, the other side
    calls acquire() to get a \l Replica. Using \l proxy() allows you to
    "observe" objects on a target device remotely via acquire, but it does not
    allow off-target \l Source objects to be acquired from the device's local:*
    network. That is where \l reverseProxy() comes in. If a proxyNode is
    created like so:

\code
    // myInternalHost is a node only visible on the device...
    QRemoteObjectHost myInternalHost("local:MyHost");

    // Regular host node, listening on port 12123, so visible to other
    // devices
    QRemoteObjectHost proxyNode("tcp://localhost:12123");

    // Enable proxying objects from nodes on the local machine's internal
    // QtRO bus.  Note the hostUrl parameter is now needed.
    proxyNode.proxy("local:registry", "local:fromProxy");
    proxyNode.reverseProxy();
\endcode

    And from another device you create another node:

\code
    // NB: localhost resolves to a different ip address than proxyNode
    QRemoteObjectHost nodeOnRemoteDevice("tcp://localhost:23234");

    // Connect to the target's proxyNode directly, or use a tcp registry...
    nodeOnRemoteDevice.connectToNode("tcp://<target device>:12123");

    // Because of the reverseProxy, we can expose objects on this device
    // and they will make their way to proxyNode...
    nodeOnRemoteDevice.enableRemoting<OtherObject>(&otherObject);
\endcode

\code
    // Acquire() can now see the objects on other devices through proxyNode,
    // due to the reverseProxy call.
    OtherObject *oo = myInternalHost.acquire<OtherObject>();
\endcode

    While the \l proxy() functionality allows \l Source objects on another
    network to be acquired(), reverseProxy() allows \l Source objects to be
    "pushed" to an otherwise inaccessible network.

    \note proxy() needs to be called before \l reverseProxy(), and a
    hostUrl needs to be provided to \l proxy for \l reverseProxy() to work. The
    \l reverseProxy() method allows a separate \a filter to be applied. This
    reverseProxy specific filter will receive notifications of new \l Source
    objects on proxyNode and acquire them on the internal node if they pass the
    reverseFilter.

    Returns \c true on success, \c false otherwise.

    \sa proxy()
*/
bool QRemoteObjectHostBase::reverseProxy(QRemoteObjectHostBase::RemoteObjectNameFilter filter)
{
    Q_D(QRemoteObjectHostBase);

    if (!d->proxyInfo) {
        qROWarning(this) << "proxy() needs to be called before setting up reverse proxy.";
        return false;
    }

    QRemoteObjectHost *host = qobject_cast<QRemoteObjectHost *>(d->proxyInfo->proxyNode);
    if (!host) {
        qROWarning(this) << "proxy() needs called with host-url to enable reverse proxy.";
        return false;
    }

    return d->proxyInfo->setReverseProxy(filter);
}

/*!
    \internal The replica needs to have a default constructor to be able
    to create a replica from QML.  In order for it to be properly
    constructed, there needs to be a way to associate the replica with a
    node and start the replica initialization.  Thus we need a public
    method on node to facilitate that.  That's initializeReplica.
*/
void QRemoteObjectNode::initializeReplica(QRemoteObjectReplica *instance, const QString &name)
{
    Q_D(QRemoteObjectNode);
    if (instance->inherits("QRemoteObjectDynamicReplica")) {
        d->setReplicaImplementation(nullptr, instance, name);
    } else {
        const QMetaObject *meta = instance->metaObject();
        // This is a templated acquire, so we tell the Source we don't need
        // them to send the class definition.  Thus we need to store the
        // metaObject for this class - if this is a nested class, the QObject
        // could be a nullptr or updated from the source,
        d->dynamicTypeManager.addFromMetaObject(meta);
        d->setReplicaImplementation(meta, instance, name.isEmpty() ? ::name(meta) : name);
    }
}

void QRemoteObjectNodePrivate::setLastError(QRemoteObjectNode::ErrorCode errorCode)
{
    Q_Q(QRemoteObjectNode);
    lastError = errorCode;
    emit q->error(lastError);
}

void QRemoteObjectNodePrivate::setReplicaImplementation(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name)
{
    qROPrivDebug() << "Starting setReplicaImplementation for" << name;
    openConnectionIfNeeded(name);
    QMutexLocker locker(&mutex);
    if (hasInstance(name)) {
        qCDebug(QT_REMOTEOBJECT)<<"setReplicaImplementation - using existing instance";
        QSharedPointer<QRemoteObjectReplicaImplementation> rep = qSharedPointerCast<QRemoteObjectReplicaImplementation>(replicas.value(name).toStrongRef());
        Q_ASSERT(rep);
        instance->d_impl = rep;
        rep->configurePrivate(instance);
    } else {
        instance->d_impl.reset(handleNewAcquire(meta, instance, name));
        instance->initialize();
        replicas.insert(name, instance->d_impl.toWeakRef());
        qROPrivDebug() << "setReplicaImplementation - Created new instance" << name<<remoteObjectAddresses();
    }
}

/*!
    Returns a pointer to the Node's \l {QRemoteObjectRegistry}, if the Node
    is using the Registry feature; otherwise it returns 0.
*/
const QRemoteObjectRegistry *QRemoteObjectNode::registry() const
{
    Q_D(const QRemoteObjectNode);
    return d->registry;
}

/*!
    \class QRemoteObjectAbstractPersistedStore
    \inmodule QtRemoteObjects
    \brief A class which provides the methods for setting PROP values of a
    replica to value they had the last time the replica was used.

    This can be used to provide a "reasonable" value to be displayed until the
    connection to the source is established and current values are available.

    This class must be overridden to provide an implementation for saving (\l
    QRemoteObjectAbstractPersistedStore::saveProperties) and restoring (\l
    QRemoteObjectAbstractPersistedStore::restoreProperties) PROP values. The derived
    type can then be set for a node, and any replica acquired from that node
    will then automatically store PERSISTED properties when the replica
    destructor is called, and retrieve the values when the replica is
    instantiated.
*/

/*!
    Constructs a QRemoteObjectAbstractPersistedStore with the given \a parent.
    The default value of \a parent is \c nullptr.
*/
QRemoteObjectAbstractPersistedStore::QRemoteObjectAbstractPersistedStore(QObject *parent)
    : QObject(parent)
{
}

QRemoteObjectAbstractPersistedStore::~QRemoteObjectAbstractPersistedStore()
{
}

/*!
    \fn virtual void QRemoteObjectAbstractPersistedStore::saveProperties(const QString &repName, const QByteArray &repSig, const QVariantList &values)

    This method will be provided the replica class's \a repName, \a repSig and the list of
    \a values that PERSISTED properties have when the replica destructor was
    called. It is the responsibility of the inheriting class to store the
    information in a manner consistent for \l
    QRemoteObjectAbstractPersistedStore::restoreProperties to retrieve.

    \sa QRemoteObjectAbstractPersistedStore::restoreProperties
*/

/*!
    \fn virtual QVariantList QRemoteObjectAbstractPersistedStore::restoreProperties(const QString &repName, const QByteArray &repSig)

    This method will be provided the replica class's \a repName and \a repSig when the
    replica is being initialized. It is the responsibility of the inheriting
    class to get the last values persisted by \l
    QRemoteObjectAbstractPersistedStore::saveProperties and return them. An empty
    QVariantList should be returned if no values are available.

    \sa QRemoteObjectAbstractPersistedStore::saveProperties
*/


QRemoteObjectAbstractPersistedStore *QRemoteObjectNode::persistedStore() const
{
    Q_D(const QRemoteObjectNode);
    return d->persistedStore;
}

/*!
    \qmlproperty QRemoteObjectAbstractPersistedStore Node::persistedStore

    Allows setting a \l QRemoteObjectAbstractPersistedStore instance for the node.

    Allows replica \l PROP members with the PERSISTED trait to save their current value when the
    replica is deleted and restore a stored value the next time the replica is started.

    Requires a \l QRemoteObjectAbstractPersistedStore class implementation to control where and how
    persistence is handled. A default QSettings-based implementation is provided by SettingsStore.
*/

/*!
    \since 5.11
    \property QRemoteObjectNode::persistedStore
    \brief Allows setting a \l QRemoteObjectAbstractPersistedStore instance for the node.

    Allows replica \l PROP members with the PERSISTED trait to save their current value when the
    replica is deleted and restore a stored value the next time the replica is started.

    Requires a \l QRemoteObjectAbstractPersistedStore class implementation to control where and how
    persistence is handled.
*/
void QRemoteObjectNode::setPersistedStore(QRemoteObjectAbstractPersistedStore *persistedStore)
{
    Q_D(QRemoteObjectNode);
    d->persistedStore = persistedStore;
}

QRemoteObjectAbstractPersistedStore::QRemoteObjectAbstractPersistedStore(QRemoteObjectAbstractPersistedStorePrivate &dptr, QObject *parent)
    : QObject(dptr, parent)
{
}

QRemoteObjectAbstractPersistedStorePrivate::QRemoteObjectAbstractPersistedStorePrivate()
{
}

QRemoteObjectAbstractPersistedStorePrivate::~QRemoteObjectAbstractPersistedStorePrivate()
{
}

QRemoteObjectMetaObjectManager::~QRemoteObjectMetaObjectManager()
{
    for (QMetaObject *mo : dynamicTypes)
        free(mo); //QMetaObjectBuilder uses malloc, not new
}

const QMetaObject *QRemoteObjectMetaObjectManager::metaObjectForType(const QString &type)
{
    qCDebug(QT_REMOTEOBJECT) << "metaObjectForType: looking for" << type << "static keys:" << staticTypes.keys() << "dynamic keys:" << dynamicTypes.keys();
    Q_ASSERT(staticTypes.contains(type) || dynamicTypes.contains(type));
    auto it = staticTypes.constFind(type);
    if (it != staticTypes.constEnd())
        return it.value();
    return dynamicTypes.value(type);
}

static void trackConnection(int typeId, IoDeviceBase *connection)
{
    QMutexLocker lock(&s_managedTypesMutex);
    if (s_trackedConnections[typeId].contains(connection))
        return;
    s_trackedConnections[typeId].insert(connection);
    auto unregisterIfNotUsed = [typeId, connection]{
        QMutexLocker lock(&s_managedTypesMutex);
        Q_ASSERT(s_trackedConnections.contains(typeId));
        Q_ASSERT(s_trackedConnections[typeId].contains(connection));
        s_trackedConnections[typeId].remove(connection);
        if (s_trackedConnections[typeId].isEmpty()) {
            s_trackedConnections.remove(typeId);
            s_managedTypes.remove(typeId);
            QMetaType::unregisterType(typeId);
        }
    };

    // Unregister the type only when the connection is destroyed
    // Do not unregister types when the connections is discconected, because
    // if it gets reconnected it will not register the types again
    QObject::connect(connection, &IoDeviceBase::destroyed, unregisterIfNotUsed);
}

struct EnumPair {
    QByteArray name;
    int value;
};

struct EnumData {
    QByteArray name;
    bool isFlag, isScoped;
    quint32 keyCount, size;
    QVector<EnumPair> values;
};

struct GadgetProperty {
    QByteArray name;
    QByteArray type;
};

struct GadgetData {
    QVector<GadgetProperty> properties;
    QVector<EnumData> enums;
};

using Gadgets = QHash<QByteArray, GadgetData>;

static void registerEnum(const QByteArray &name, const QMetaObject *meta, int size=4)
{
    // When we add support for enum classes, we will need to set this to something like
    // QByteArray(enumClass).append("::").append(enumMeta.name()) when enumMeta.isScoped() is true.
    // That is a new feature, though.
    if (QMetaType::isRegistered(QMetaType::type(name)))
        return;
    static const auto flags = QMetaType::IsEnumeration | QMetaType::NeedsConstruction | QMetaType::NeedsDestruction;
    int id;
    switch (size) {
    case 1: id = QMetaType::registerType(name.constData(), nullptr, nullptr, &EnumDestructor<qint8>,
                                                 &EnumConstructor<qint8>, size, flags, meta);
        break;
    case 2: id = QMetaType::registerType(name.constData(), nullptr, nullptr, &EnumDestructor<qint16>,
                                                 &EnumConstructor<qint16>, size, flags, meta);
        break;
    case 4: id = QMetaType::registerType(name.constData(), nullptr, nullptr, &EnumDestructor<qint32>,
                                                 &EnumConstructor<qint32>, size, flags, meta);
        break;
    // Qt currently only supports enum values of 4 or less bytes (QMetaEnum value(index) returns int)
//    case 8: id = QMetaType::registerType(name.constData(), nullptr, nullptr, &EnumDestructor<qint64>,
//                                                 &EnumConstructor<qint64>, size, flags, meta);
//        break;
    default:
        qWarning() << "Invalid enum detected" << name << "with size" << size << ".  Defaulting to register as int.";
        size = 4;
        id = QMetaType::registerType(name.constData(), nullptr, nullptr, &EnumDestructor<qint32>,
                                                 &EnumConstructor<qint32>, size, flags, meta);
    }
#ifdef QTRO_VERBOSE_PROTOCOL
    qDebug() << "Registering new enum with id" << id << name << "size:" << size;
#endif
    qCDebug(QT_REMOTEOBJECT) << "Registering new enum with id" << id << name << "size:" << size;
}

static int registerGadgets(IoDeviceBase *connection, Gadgets &gadgets, QByteArray typeName)
{
   const auto &gadget = gadgets.take(typeName);
   int typeId = QMetaType::type(typeName);
   if (typeId != QMetaType::UnknownType) {
       trackConnection(typeId, connection);
       return typeId;
   }

   QMetaObjectBuilder gadgetBuilder;
   gadgetBuilder.setClassName(typeName);
   gadgetBuilder.setFlags(QMetaObjectBuilder::DynamicMetaObject | QMetaObjectBuilder::PropertyAccessInStaticMetaCall);
   GadgetType gadgetType;
   for (const auto &prop : gadget.properties) {
       int propertyType = QMetaType::type(prop.type);
       if (!propertyType && gadgets.contains(prop.type))
           propertyType = registerGadgets(connection, gadgets, prop.type);
       gadgetType.push_back(QVariant(QVariant::Type(propertyType)));
       auto dynamicProperty = gadgetBuilder.addProperty(prop.name, prop.type);
       dynamicProperty.setWritable(true);
       dynamicProperty.setReadable(true);
   }
   for (const auto &enumData: gadget.enums) {
       auto enumBuilder = gadgetBuilder.addEnumerator(enumData.name);
       enumBuilder.setIsFlag(enumData.isFlag);
       enumBuilder.setIsScoped(enumData.isScoped);

       for (quint32 k = 0; k < enumData.keyCount; ++k) {
           const auto pair = enumData.values.at(k);
           enumBuilder.addKey(pair.name, pair.value);
       }
   }
   auto meta = gadgetBuilder.toMetaObject();
   const auto enumCount = meta->enumeratorCount();
   for (int i = 0; i < enumCount; i++) {
       const QByteArray registeredName = QByteArray(typeName).append("::").append(meta->enumerator(i).name());
       registerEnum(registeredName, meta, gadget.enums.at(i).size);
   }
   QMetaType::TypeFlags flags = QMetaType::IsGadget;
   int gadgetTypeId;
   if (meta->propertyCount()) {
       meta->d.static_metacall = &GadgetsStaticMetacallFunction;
       meta->d.superdata = nullptr;
       flags |= QMetaType::NeedsConstruction | QMetaType::NeedsDestruction;
       gadgetTypeId = QMetaType::registerType(typeName.constData(),
                                              &GadgetTypedDestructor,
                                              &GadgetTypedConstructor,
                                              sizeof(GadgetType),
                                              flags, meta);
       QMetaType::registerStreamOperators(gadgetTypeId, &GadgetSaveOperator, &GadgetLoadOperator);
   } else {
       gadgetTypeId = QMetaType::registerType(typeName.constData(),
                                              nullptr,
                                              nullptr,
                                              sizeof(GadgetType),
                                              flags, meta);
   }
   trackConnection(gadgetTypeId, connection);
   QMutexLocker lock(&s_managedTypesMutex);
   s_managedTypes[gadgetTypeId] = qMakePair(gadgetType, std::shared_ptr<QMetaObject>{meta, [](QMetaObject *ptr){ ::free(ptr); }});
   return gadgetTypeId;
}

static void registerAllGadgets(IoDeviceBase *connection, Gadgets &gadgets)
{
    while (!gadgets.isEmpty())
        registerGadgets(connection, gadgets, gadgets.constBegin().key());
}

static void deserializeEnum(QDataStream &ds, EnumData &enumData)
{
    ds >> enumData.name;
    ds >> enumData.isFlag;
    ds >> enumData.isScoped;
    ds >> enumData.size;
    ds >> enumData.keyCount;
    for (quint32 i = 0; i < enumData.keyCount; i++) {
        EnumPair pair;
        ds >> pair.name;
        ds >> pair.value;
        enumData.values.push_back(pair);
    }
}

static void parseGadgets(IoDeviceBase *connection, QDataStream &in)
{
    quint32 qtEnums, numGadgets;
    in >> qtEnums; // Qt enums - just need registration
    for (quint32 i = 0; i < qtEnums; ++i) {
        QByteArray enumName;
        in >> enumName;
        QMetaType t(QMetaType::type(enumName.constData()));
        registerEnum(enumName, t.metaObject()); // All Qt enums have default type int
    }
    in >> numGadgets;
    if (numGadgets == 0)
        return;
    Gadgets gadgets;
    for (quint32 i = 0; i < numGadgets; ++i) {
        QByteArray type;
        in >> type;
        quint32 numProperties, numEnums;
        in >> numProperties;
        auto &properties = gadgets[type].properties;
        for (quint32 p = 0; p < numProperties; ++p) {
            GadgetProperty prop;
            in >> prop.name;
            in >> prop.type;
            properties.push_back(prop);
        }
        in >> numEnums;
        auto &enums = gadgets[type].enums;
        for (quint32 e = 0; e < numEnums; ++e) {
            EnumData enumData;
            deserializeEnum(in, enumData);
            enums.push_back(enumData);
        }
    }
    registerAllGadgets(connection, gadgets);
}

QMetaObject *QRemoteObjectMetaObjectManager::addDynamicType(IoDeviceBase *connection, QDataStream &in)
{
    QMetaObjectBuilder builder;
    builder.setSuperClass(&QRemoteObjectReplica::staticMetaObject);
    builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);

    QString typeString;
    QByteArray type;
    quint32 numEnums = 0;
    quint32 numSignals = 0;
    quint32 numMethods = 0;
    quint32 numProperties = 0;

    in >> typeString;
    type = typeString.toLatin1();
    builder.addClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE, type);
    builder.setClassName(type);

    in >> numEnums;
    QVector<quint32> enumSizes(numEnums);
    for (quint32 i = 0; i < numEnums; ++i) {
        EnumData enumData;
        deserializeEnum(in, enumData);
        auto enumBuilder = builder.addEnumerator(enumData.name);
        enumBuilder.setIsFlag(enumData.isFlag);
        enumBuilder.setIsScoped(enumData.isScoped);
        enumSizes[i] = enumData.size;

        for (quint32 k = 0; k < enumData.keyCount; ++k) {
            const auto pair = enumData.values.at(k);
            enumBuilder.addKey(pair.name, pair.value);
        }
    }
    parseGadgets(connection, in);

    int curIndex = 0;

    in >> numSignals;
    for (quint32 i = 0; i < numSignals; ++i) {
        QByteArray signature;
        QList<QByteArray> paramNames;
        in >> signature;
        in >> paramNames;
        ++curIndex;
        auto mmb = builder.addSignal(signature);
        mmb.setParameterNames(paramNames);
    }

    in >> numMethods;
    for (quint32 i = 0; i < numMethods; ++i) {
        QByteArray signature, returnType;
        QList<QByteArray> paramNames;
        in >> signature;
        in >> returnType;
        in >> paramNames;
        ++curIndex;
        const bool isVoid = returnType.isEmpty() || returnType == QByteArrayLiteral("void");
        QMetaMethodBuilder mmb;
        if (isVoid)
            mmb = builder.addMethod(signature);
        else
            mmb = builder.addMethod(signature, QByteArrayLiteral("QRemoteObjectPendingCall"));
        mmb.setParameterNames(paramNames);
    }

    in >> numProperties;

    for (quint32 i = 0; i < numProperties; ++i) {
        QByteArray name;
        QByteArray typeName;
        QByteArray signalName;
        in >> name;
        in >> typeName;
        in >> signalName;
        if (signalName.isEmpty())
            builder.addProperty(name, typeName);
        else
            builder.addProperty(name, typeName, builder.indexOfSignal(signalName));
    }

    auto meta = builder.toMetaObject();
    // Our type likely has enumerations from the inherited base classes, such as the Replica State
    // We only want to register the new enumerations, and since we just added them, we know they
    // are the last indices.  Thus a backwards count seems most efficient.
    const int totalEnumCount = meta->enumeratorCount();
    int incrementingIndex = 0;
    for (int i = numEnums; i > 0; i--) {
        auto const enumMeta = meta->enumerator(totalEnumCount - i);
        const QByteArray registeredName = QByteArray(type).append("::").append(enumMeta.name());
        registerEnum(registeredName, meta, enumSizes.at(incrementingIndex++));
    }
    dynamicTypes.insert(typeString, meta);
    return meta;
}

void QRemoteObjectMetaObjectManager::addFromMetaObject(const QMetaObject *metaObject)
{
    QString className = QLatin1String(metaObject->className());
    if (!className.endsWith(QLatin1String("Replica")))
        return;
    if (className == QLatin1String("QRemoteObjectDynamicReplica") || staticTypes.contains(className))
        return;
    className.chop(7); //Remove 'Replica' from name
    staticTypes.insert(className, metaObject);
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

    if (!initConnection(remoteObjectAddresses().value(name).hostUrl))
        qROPrivWarning() << "failed to open connection to" << name;
}

bool QRemoteObjectNodePrivate::initConnection(const QUrl &address)
{
    Q_Q(QRemoteObjectNode);
    if (requestedUrls.contains(address)) {
        qROPrivDebug() << "Connection already requested for " << address.toString();
        return true;
    }

    requestedUrls.insert(address);

    if (schemaHandlers.contains(address.scheme())) {
        schemaHandlers[address.scheme()](address);
        return true;
    }

    ClientIoDevice *connection = QtROClientFactory::instance()->create(address, q);
    if (!connection) {
        qROPrivWarning() << "Could not create ClientIoDevice for client. Invalid url/scheme provided?" << address;
        return false;
    }
    qROPrivDebug() << "Opening connection to" << address.toString();
    qROPrivDebug() << "Replica Connection isValid" << connection->isOpen();
    QObject::connect(connection, &ClientIoDevice::shouldReconnect, q, [this, connection]() {
        onShouldReconnect(connection);
    });
    QObject::connect(connection, &IoDeviceBase::readyRead, q, [this, connection]() {
        onClientRead(connection);
    });
    connection->connectToServer();

    return true;
}

bool QRemoteObjectNodePrivate::hasInstance(const QString &name)
{
    if (!replicas.contains(name))
        return false;

    QSharedPointer<QReplicaImplementationInterface> rep = replicas.value(name).toStrongRef();
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
        registry->d_impl->setProperty(0, QVariant::fromValue(locs));
        qROPrivDebug() << "onRemoteObjectSourceAdded, now locations =" << locs;
    }
    if (replicas.contains(entry.first)) //We have a replica waiting on this remoteObject
    {
        QSharedPointer<QReplicaImplementationInterface> rep = replicas.value(entry.first).toStrongRef();
        if (!rep) { //replica has been deleted, remove from list
            replicas.remove(entry.first);
            return;
        }

        initConnection(entry.second.hostUrl);

        qROPrivDebug() << "Called initConnection due to new RemoteObjectSource added via registry" << entry.first;
    }
}

void QRemoteObjectNodePrivate::onRemoteObjectSourceRemoved(const QRemoteObjectSourceLocation &entry)
{
    if (!entry.first.isEmpty()) {
        QRemoteObjectSourceLocations locs = registry->sourceLocations();
        locs.remove(entry.first);
        registry->d_impl->setProperty(0, QVariant::fromValue(locs));
    }
}

void QRemoteObjectNodePrivate::onRegistryInitialized()
{
    qROPrivDebug() << "Registry Initialized" << remoteObjectAddresses();

    const auto remotes = remoteObjectAddresses();
    for (auto i = remotes.cbegin(), end = remotes.cend(); i != end; ++i) {
        if (replicas.contains(i.key())) //We have a replica waiting on this remoteObject
        {
            QSharedPointer<QReplicaImplementationInterface> rep = replicas.value(i.key()).toStrongRef();
            if (rep && !requestedUrls.contains(i.value().hostUrl))
                initConnection(i.value().hostUrl);
            else if (!rep) //replica has been deleted, remove from list
                replicas.remove(i.key());

            continue;
        }
    }
}

void QRemoteObjectNodePrivate::onShouldReconnect(ClientIoDevice *ioDevice)
{
    Q_Q(QRemoteObjectNode);

    const auto remoteObjects = ioDevice->remoteObjects();
    for (const QString &remoteObject : remoteObjects) {
        connectedSources.remove(remoteObject);
        ioDevice->removeSource(remoteObject);
        if (replicas.contains(remoteObject)) { //We have a replica waiting on this remoteObject
            QSharedPointer<QConnectedReplicaImplementation> rep = qSharedPointerCast<QConnectedReplicaImplementation>(replicas.value(remoteObject).toStrongRef());
            if (rep && !rep->connectionToSource.isNull()) {
                rep->setDisconnected();
            } else if (!rep) {
                replicas.remove(remoteObject);
            }
        }
    }
    if (requestedUrls.contains(ioDevice->url())) {
        // Only try to reconnect to URLs requested via connectToNode
        // If we connected via registry, wait for the registry to see the node/source again
        pendingReconnect.insert(ioDevice);
        if (!reconnectTimer.isActive()) {
            reconnectTimer.start(retryInterval, q);
            qROPrivDebug() << "Starting reconnect timer";
        }
    } else {
        qROPrivDebug() << "Url" << ioDevice->url().toDisplayString().toLatin1()
                       << "lost.  We will reconnect Replicas if they reappear on the Registry.";
    }
}

//This version of handleNewAcquire creates a QConnectedReplica. If this is a
//host node, the QRemoteObjectHostBasePrivate overload is called instead.
QReplicaImplementationInterface *QRemoteObjectNodePrivate::handleNewAcquire(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name)
{
    Q_Q(QRemoteObjectNode);
    QConnectedReplicaImplementation *rp = new QConnectedReplicaImplementation(name, meta, q);
    rp->configurePrivate(instance);
    if (connectedSources.contains(name)) { //Either we have a peer connections, or existing connection via registry
        handleReplicaConnection(connectedSources[name].objectSignature, rp, connectedSources[name].device);
    } else {
        //No existing connection, but we know we can connect via registry
        const auto &sourceLocations = remoteObjectAddresses();
        const auto it = sourceLocations.constFind(name);
        // This will try the connection, and if successful, the remoteObjects will be sent
        // The link to the replica will be handled then
        if (it != sourceLocations.constEnd())
            initConnection(it.value().hostUrl);
    }
    return rp;
}

void QRemoteObjectNodePrivate::handleReplicaConnection(const QString &name)
{
    QSharedPointer<QRemoteObjectReplicaImplementation> rep = qSharedPointerCast<QRemoteObjectReplicaImplementation>(replicas.value(name).toStrongRef());
    if (!rep) { //replica has been deleted, remove from list
        replicas.remove(name);
        return;
    }

    if (rep->isShortCircuit())
        return;

    QConnectedReplicaImplementation *connectedRep = static_cast<QConnectedReplicaImplementation *>(rep.data());
    if (connectedRep->connectionToSource.isNull()) {
        const auto sourceInfo = connectedSources.value(name);
        handleReplicaConnection(sourceInfo.objectSignature, connectedRep, sourceInfo.device);
    }
}

void QRemoteObjectNodePrivate::handleReplicaConnection(const QByteArray &sourceSignature, QConnectedReplicaImplementation *rep, IoDeviceBase *connection)
{
    if (!checkSignatures(rep->m_objectSignature, sourceSignature)) {
        qROPrivWarning() << "Signature mismatch for" << rep->m_metaObject->className() << (rep->m_objectName.isEmpty() ? QLatin1String("(unnamed)") : rep->m_objectName);
        rep->setState(QRemoteObjectReplica::SignatureMismatch);
        return;
    }
    rep->setConnection(connection);
}

//Host Nodes can use the more efficient InProcess Replica if we (this Node) hold the Source for the
//requested Replica.  If not, fall back to the Connected Replica case.
QReplicaImplementationInterface *QRemoteObjectHostBasePrivate::handleNewAcquire(const QMetaObject *meta, QRemoteObjectReplica *instance, const QString &name)
{
    QMap<QString, QRemoteObjectSourceBase*>::const_iterator mapIt;
    if (remoteObjectIo && map_contains(remoteObjectIo->m_sourceObjects, name, mapIt)) {
        Q_Q(QRemoteObjectHostBase);
        QInProcessReplicaImplementation *rp = new QInProcessReplicaImplementation(name, meta, q);
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
    IoDeviceBase *connection = qobject_cast<IoDeviceBase*>(obj);
    QRemoteObjectPacketTypeEnum packetType;
    Q_ASSERT(connection);

    do {
        if (!connection->read(packetType, rxName))
            return;

        if (packetType != Handshake && !m_handshakeReceived) {
            qROPrivWarning() << "Expected Handshake, got " << packetType;
            setLastError(QRemoteObjectNode::ProtocolMismatch);
            connection->close();
            break;
        }

        switch (packetType) {
        case Pong:
        {
            QSharedPointer<QRemoteObjectReplicaImplementation> rep = qSharedPointerCast<QRemoteObjectReplicaImplementation>(replicas.value(rxName).toStrongRef());
            if (rep)
                rep->notifyAboutReply(0, {});
            else //replica has been deleted, remove from list
                replicas.remove(rxName);
            break;
        }
        case Handshake:
            if (rxName != QtRemoteObjects::protocolVersion) {
                qWarning() << "*** Protocol Mismatch, closing connection ***. Got" << rxName << "expected" << QtRemoteObjects::protocolVersion;
                setLastError(QRemoteObjectNode::ProtocolMismatch);
                connection->close();
            } else {
                m_handshakeReceived = true;
            }
            break;
        case ObjectList:
        {
            deserializeObjectListPacket(connection->stream(), rxObjects);
            qROPrivDebug() << "newObjects:" << rxObjects;
            // We need to make sure all of the source objects are in connectedSources before we add connections,
            // otherwise nested QObjects could fail (we want to acquire children before parents, and the object
            // list is unordered)
            for (const auto &remoteObject : qAsConst(rxObjects)) {
                qROPrivDebug() << "  connectedSources.contains(" << remoteObject << ")" << connectedSources.contains(remoteObject.name) << replicas.contains(remoteObject.name);
                if (!connectedSources.contains(remoteObject.name)) {
                    connectedSources[remoteObject.name] = SourceInfo{connection, remoteObject.typeName, remoteObject.signature};
                    connection->addSource(remoteObject.name);
                    // Make sure we handle Registry first if it is available
                    if (remoteObject.name == QLatin1String("Registry") && replicas.contains(remoteObject.name))
                        handleReplicaConnection(remoteObject.name);
                }
            }
            for (const auto &remoteObject : qAsConst(rxObjects)) {
                if (replicas.contains(remoteObject.name)) //We have a replica waiting on this remoteObject
                    handleReplicaConnection(remoteObject.name);
            }
            break;
        }
        case InitPacket:
        {
            qROPrivDebug() << "InitPacket-->" << rxName << this;
            QSharedPointer<QConnectedReplicaImplementation> rep = qSharedPointerCast<QConnectedReplicaImplementation>(replicas.value(rxName).toStrongRef());
            //Use m_rxArgs (a QVariantList to hold the properties QVariantList)
            deserializeInitPacket(connection->stream(), rxArgs);
            if (rep)
            {
                handlePointerToQObjectProperties(rep.data(), rxArgs);
                rep->initialize(rxArgs);
            } else { //replica has been deleted, remove from list
                replicas.remove(rxName);
            }
            break;
        }
        case InitDynamicPacket:
        {
            qROPrivDebug() << "InitDynamicPacket-->" << rxName << this;
            const QMetaObject *meta = dynamicTypeManager.addDynamicType(connection, connection->stream());
            deserializeInitPacket(connection->stream(), rxArgs);
            QSharedPointer<QConnectedReplicaImplementation> rep = qSharedPointerCast<QConnectedReplicaImplementation>(replicas.value(rxName).toStrongRef());
            if (rep)
            {
                rep->setDynamicMetaObject(meta);
                handlePointerToQObjectProperties(rep.data(), rxArgs);
                rep->setDynamicProperties(rxArgs);
            } else { //replica has been deleted, remove from list
                replicas.remove(rxName);
            }
            break;
        }
        case RemoveObject:
        {
            qROPrivDebug() << "RemoveObject-->" << rxName << this;
            connectedSources.remove(rxName);
            connection->removeSource(rxName);
            if (replicas.contains(rxName)) { //We have a replica using the removed source
                QSharedPointer<QConnectedReplicaImplementation> rep = qSharedPointerCast<QConnectedReplicaImplementation>(replicas.value(rxName).toStrongRef());
                if (rep && !rep->connectionToSource.isNull()) {
                    rep->connectionToSource.clear();
                    rep->setState(QRemoteObjectReplica::Suspect);
                } else if (!rep) {
                    replicas.remove(rxName);
                }
            }
            break;
        }
        case PropertyChangePacket:
        {
            int propertyIndex;
            deserializePropertyChangePacket(connection->stream(), propertyIndex, rxValue);
            QSharedPointer<QRemoteObjectReplicaImplementation> rep = qSharedPointerCast<QRemoteObjectReplicaImplementation>(replicas.value(rxName).toStrongRef());
            if (rep) {
                QConnectedReplicaImplementation *connectedRep = nullptr;
                if (!rep->isShortCircuit()) {
                    connectedRep = static_cast<QConnectedReplicaImplementation *>(rep.data());
                    if (!connectedRep->childIndices().contains(propertyIndex))
                        connectedRep = nullptr; //connectedRep will be a valid pointer only if propertyIndex is a child index
                }
                if (connectedRep)
                    rep->setProperty(propertyIndex, handlePointerToQObjectProperty(connectedRep, propertyIndex, rxValue));
                else {
                    const QMetaProperty property = rep->m_metaObject->property(propertyIndex + rep->m_metaObject->propertyOffset());
                    if (property.userType() == QMetaType::QVariant && rxValue.canConvert<QRO_>()) {
                        // This is a type that requires registration
                        QRO_ typeInfo = rxValue.value<QRO_>();
                        QDataStream in(typeInfo.classDefinition);
                        parseGadgets(connection, in);
                        QDataStream ds(typeInfo.parameters);
                        ds >> rxValue;
                    }
                    rep->setProperty(propertyIndex, decodeVariant(rxValue, property.userType()));
                }
            } else { //replica has been deleted, remove from list
                replicas.remove(rxName);
            }
            break;
        }
        case InvokePacket:
        {
            int call, index, serialId, propertyIndex;
            deserializeInvokePacket(connection->stream(), call, index, rxArgs, serialId, propertyIndex);
            QSharedPointer<QRemoteObjectReplicaImplementation> rep = qSharedPointerCast<QRemoteObjectReplicaImplementation>(replicas.value(rxName).toStrongRef());
            if (rep) {
                static QVariant null(QMetaType::QObjectStar, (void*)0);
                QVariant paramValue;
                // Qt usually supports 9 arguments, so ten should be usually safe
                QVarLengthArray<void*, 10> param(rxArgs.size() + 1);
                param[0] = null.data(); //Never a return value
                if (rxArgs.size()) {
                    auto signal = rep->m_metaObject->method(index+rep->m_signalOffset);
                    for (int i = 0; i < rxArgs.size(); i++) {
                        if (signal.parameterType(i) == QMetaType::QVariant)
                            param[i + 1] = const_cast<void*>(reinterpret_cast<const void*>(&rxArgs.at(i)));
                        else {
                            decodeVariant(rxArgs[i], signal.parameterType(i));
                            param[i + 1] = const_cast<void *>(rxArgs.at(i).data());
                        }
                    }
                } else if (propertyIndex != -1) {
                    param.resize(2);
                    paramValue = rep->getProperty(propertyIndex);
                    param[1] = paramValue.data();
                }
                qROPrivDebug() << "Replica Invoke-->" << rxName << rep->m_metaObject->method(index+rep->m_signalOffset).name() << index << rep->m_signalOffset;
                // We activate on rep->metaobject() so the private metacall is used, not m_metaobject (which
                // is the class thie replica looks like)
                QMetaObject::activate(rep.data(), rep->metaObject(), index+rep->m_signalOffset, param.data());
            } else { //replica has been deleted, remove from list
                replicas.remove(rxName);
            }
            break;
        }
        case InvokeReplyPacket:
        {
            int ackedSerialId;
            deserializeInvokeReplyPacket(connection->stream(), ackedSerialId, rxValue);
            QSharedPointer<QRemoteObjectReplicaImplementation> rep = qSharedPointerCast<QRemoteObjectReplicaImplementation>(replicas.value(rxName).toStrongRef());
            if (rep) {
                qROPrivDebug() << "Received InvokeReplyPacket ack'ing serial id:" << ackedSerialId;
                rep->notifyAboutReply(ackedSerialId, rxValue);
            } else { //replica has been deleted, remove from list
                replicas.remove(rxName);
            }
            break;
        }
        case AddObject:
        case Invalid:
        case Ping:
            qROPrivWarning() << "Unexpected packet received";
        }
    } while (connection->bytesAvailable()); // have bytes left over, so do another iteration
}

/*!
    \class QRemoteObjectNode
    \inmodule QtRemoteObjects
    \brief A node on a Qt Remote Objects network.

    The QRemoteObjectNode class provides an entry point to a QtRemoteObjects
    network. A network can be as simple as two nodes, or an arbitrarily complex
    set of processes and devices.

    A QRemoteObjectNode does not have a url that other nodes can connect to,
    and thus is able to acquire replicas only. It is not able to share source
    objects (only QRemoteObjectHost and QRemoteObjectRegistryHost Nodes can
    share).

    Nodes may connect to each other directly using \l connectToNode, or
    they can use the QRemoteObjectRegistry to simplify connections.

    The QRemoteObjectRegistry is a special replica available to every node that
    connects to the Registry Url. It knows how to connect to every
    QRemoteObjectSource object on the network.

    \sa QRemoteObjectHost, QRemoteObjectRegistryHost
*/

/*!
    \class QRemoteObjectHostBase
    \inmodule QtRemoteObjects
    \brief The QRemoteObjectHostBase class provides base functionality common to \l
    {QRemoteObjectHost} {Host} and \l {QRemoteObjectRegistryHost} {RegistryHost} classes.

    QRemoteObjectHostBase is a base class that cannot be instantiated directly.
    It provides the enableRemoting and disableRemoting functionality shared by
    all host nodes (\l {QRemoteObjectHost} {Host} and \l
    {QRemoteObjectRegistryHost} {RegistryHost}) as well as the logic required
    to expose \l {Source} objects on the Remote Objects network.
*/

/*!
    \class QRemoteObjectHost
    \inmodule QtRemoteObjects
    \brief A (Host) Node on a Qt Remote Objects network.

    The QRemoteObjectHost class provides an entry point to a QtRemoteObjects
    network. A network can be as simple as two nodes, or an arbitrarily complex
    set of processes and devices.

    QRemoteObjectHosts have the same capabilities as QRemoteObjectNodes, but
    they can also be connected to and can share source objects on the network.

    Nodes may connect to each other directly using \l connectToNode, or they
    can use the QRemoteObjectRegistry to simplify connections.

    The QRemoteObjectRegistry is a special replica available to every node that
    connects to the registry Url. It knows how to connect to every
    QRemoteObjectSource object on the network.

    \sa QRemoteObjectNode, QRemoteObjectRegistryHost
*/

/*!
    \class QRemoteObjectRegistryHost
    \inmodule QtRemoteObjects
    \brief A (Host/Registry) node on a Qt Remote Objects network.

    The QRemoteObjectRegistryHost class provides an entry point to a QtRemoteObjects
    network. A network can be as simple as two Nodes, or an arbitrarily complex
    set of processes and devices.

    A QRemoteObjectRegistryHost has the same capability that a
    QRemoteObjectHost has (which includes everything a QRemoteObjectNode
    supports), and in addition is the owner of the Registry. Any
    QRemoteObjectHost node that connects to this Node will have all of their
    Source objects made available by the Registry.

    Nodes only support connection to one \l registry, calling \l
    QRemoteObjectNode::setRegistryUrl when a Registry is already set is
    considered an error. For something like a secure and insecure network
    (where different Registries would be applicable), the recommendation is to
    create separate Nodes to connect to each, in effect creating two
    independent Qt Remote Objects networks.

    Nodes may connect to each other directly using \l connectToNode, or they
    can use the QRemoteObjectRegistry to simplify connections.

    The QRemoteObjectRegistry is a special Replica available to every Node that
    connects to the Registry Url. It knows how to connect to every
    QRemoteObjectSource object on the network.

    \sa QRemoteObjectNode, QRemoteObjectHost
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
    \value HostUrlInvalid The given url has an invalid or unrecognized scheme.
    \value ProtocolMismatch The client and the server have different protocol versions.
    \value ListenFailed Can't listen on the specified host port.
*/

/*!
    \enum QRemoteObjectHostBase::AllowedSchemas

    This enum is used to specify whether a Node will accept a url with an
    unrecognized schema for the hostUrl. By default only urls with known
    schemas are accepted, but using \c AllowExternalRegistration will enable
    the \l Registry to pass your external (to QtRO) url to client Nodes.

    \value BuiltInSchemasOnly Only allow the hostUrl to be set to a QtRO
    supported schema. This is the default value, and causes a Node error to be
    set if an unrecognized schema is provided.
    \value AllowExternalRegistration The provided schema is registered as an
    \l {External Schemas}{External Schema}

    \sa QRemoteObjectHost
*/

/*!
    \fn ObjectType *QRemoteObjectNode::acquire(const QString &name)

    Returns a pointer to a Replica of type ObjectType (which is a template
    parameter and must inherit from \l QRemoteObjectReplica). That is, the
    template parameter must be a \l {repc} generated type. The \a name
    parameter can be used to specify the \a name given to the object
    during the QRemoteObjectHost::enableRemoting() call.
*/

void QRemoteObjectNodePrivate::initialize()
{
    qRegisterMetaType<QRemoteObjectNode *>();
    qRegisterMetaType<QRemoteObjectNode::ErrorCode>();
    qRegisterMetaType<QAbstractSocket::SocketError>(); //For queued qnx error()
    qRegisterMetaTypeStreamOperators<QVector<int> >();
    qRegisterMetaTypeStreamOperators<QRemoteObjectPackets::QRO_>();
    // To support dynamic MODELs, we need to make sure the types are registered
    QAbstractItemModelSourceAdapter::registerTypes();
}

bool QRemoteObjectNodePrivate::checkSignatures(const QByteArray &a, const QByteArray &b)
{
    // if any of a or b is empty it means it's a dynamic ojects or an item model
    if (a.isEmpty() || b.isEmpty())
        return true;
    return a == b;
}


void QRemoteObjectNode::persistProperties(const QString &repName, const QByteArray &repSig, const QVariantList &props)
{
    Q_D(QRemoteObjectNode);
    if (d->persistedStore) {
        d->persistedStore->saveProperties(repName, repSig, props);
    } else {
        qCWarning(QT_REMOTEOBJECT) << qPrintable(objectName()) << "Unable to store persisted properties for" << repName;
        qCWarning(QT_REMOTEOBJECT) << "    No persisted store set.";
    }
}

QVariantList QRemoteObjectNode::retrieveProperties(const QString &repName, const QByteArray &repSig)
{
    Q_D(QRemoteObjectNode);
    if (d->persistedStore) {
        return d->persistedStore->restoreProperties(repName, repSig);
    }
    qCWarning(QT_REMOTEOBJECT) << qPrintable(objectName()) << "Unable to retrieve persisted properties for" << repName;
    qCWarning(QT_REMOTEOBJECT) << "    No persisted store set.";
    return QVariantList();
}

/*!
    Default constructor for QRemoteObjectNode with the given \a parent. A Node
    constructed in this manner can not be connected to, and thus can not expose
    Source objects on the network. It also will not include a \l
    QRemoteObjectRegistry, unless set manually using setRegistryUrl.

    \sa connectToNode, setRegistryUrl
*/
QRemoteObjectNode::QRemoteObjectNode(QObject *parent)
    : QObject(*new QRemoteObjectNodePrivate, parent)
{
    Q_D(QRemoteObjectNode);
    d->initialize();
}

/*!
    QRemoteObjectNode connected to a {QRemoteObjectRegistry} {Registry}. A Node
    constructed in this manner can not be connected to, and thus can not expose
    Source objects on the network. Finding and connecting to other (Host) Nodes
    is handled by the QRemoteObjectRegistry specified by \a registryAddress.

    \sa connectToNode, setRegistryUrl, QRemoteObjectHost, QRemoteObjectRegistryHost
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
    \qmltype Host
    \instantiates QRemoteObjectHost
    \inqmlmodule QtRemoteObjects
    \brief A host node on a Qt Remote Objects network.

    The Host type provides an entry point to a Qt Remote Objects network. A network
    can be as simple as two nodes, or an arbitrarily complex set of processes and devices.

    Hosts have the same capabilities as Nodes, but they can also be connected to and can
    share source objects on the network.
*/

/*!
    \internal This is a base class for both QRemoteObjectHost and
    QRemoteObjectRegistryHost to provide the shared features/functions for
    sharing \l Source objects.
*/
QRemoteObjectHostBase::QRemoteObjectHostBase(QRemoteObjectHostBasePrivate &d, QObject *parent)
    : QRemoteObjectNode(d, parent)
{ }

/*!
    Constructs a new QRemoteObjectHost Node (i.e., a Node that supports
    exposing \l Source objects on the QtRO network) with the given \a parent.
    This constructor is meant specific to support QML in the future as it will
    not be available to connect to until \l {QRemoteObjectHost::}{setHostUrl} is called.

    \sa setHostUrl(), setRegistryUrl()
*/
QRemoteObjectHost::QRemoteObjectHost(QObject *parent)
    : QRemoteObjectHostBase(*new QRemoteObjectHostPrivate, parent)
{ }

/*!
    Constructs a new QRemoteObjectHost Node (i.e., a Node that supports
    exposing \l Source objects on the QtRO network) with address \a address. If
    set, \a registryAddress will be used to connect to the \l
    QRemoteObjectRegistry at the provided address. The \a allowedSchemas
    parameter is only needed (and should be set to \l
    {QRemoteObjectHostBase::AllowExternalRegistration}
    {AllowExternalRegistration}) if the schema of the url should be used as an
    \l {External Schemas} {External Schema} by the registry.

    \sa setHostUrl(), setRegistryUrl()
*/
QRemoteObjectHost::QRemoteObjectHost(const QUrl &address, const QUrl &registryAddress,
                                     AllowedSchemas allowedSchemas, QObject *parent)
    : QRemoteObjectHostBase(*new QRemoteObjectHostPrivate, parent)
{
    if (!address.isEmpty()) {
        if (!setHostUrl(address, allowedSchemas))
            return;
    }

    if (!registryAddress.isEmpty())
        setRegistryUrl(registryAddress);
}

/*!
    Constructs a new QRemoteObjectHost Node (i.e., a Node that supports
    exposing \l Source objects on the QtRO network) with a url of \a
    address and the given \a parent. This overload is provided as a convenience for specifying a
    QObject parent without providing a registry address.

    \sa setHostUrl(), setRegistryUrl()
*/
QRemoteObjectHost::QRemoteObjectHost(const QUrl &address, QObject *parent)
    : QRemoteObjectHostBase(*new QRemoteObjectHostPrivate, parent)
{
    if (!address.isEmpty())
        setHostUrl(address);
}

/*!
    \internal QRemoteObjectHost::QRemoteObjectHost
*/
QRemoteObjectHost::QRemoteObjectHost(QRemoteObjectHostPrivate &d, QObject *parent)
    : QRemoteObjectHostBase(d, parent)
{ }

QRemoteObjectHost::~QRemoteObjectHost() {}

/*!
    Constructs a new QRemoteObjectRegistryHost Node with the given \a parent. RegistryHost Nodes have
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

/*!
    \internal
*/
QRemoteObjectRegistryHost::QRemoteObjectRegistryHost(QRemoteObjectRegistryHostPrivate &d, QObject *parent)
    : QRemoteObjectHostBase(d, parent)
{ }

QRemoteObjectRegistryHost::~QRemoteObjectRegistryHost() {}

QRemoteObjectNode::~QRemoteObjectNode()
{ }

QRemoteObjectHostBase::~QRemoteObjectHostBase()
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

/*!
    Similar to QObject::setObjectName() (which this method calls), but this
    version also applies the \a name to internal classes as well, which are
    used in some of the debugging output.
*/
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
bool QRemoteObjectHostBase::setHostUrl(const QUrl &hostAddress, AllowedSchemas allowedSchemas)
{
    Q_D(QRemoteObjectHostBase);
    if (d->remoteObjectIo) {
        d->setLastError(ServerAlreadyCreated);
        return false;
    }

    if (allowedSchemas == AllowedSchemas::BuiltInSchemasOnly && !QtROServerFactory::instance()->isValid(hostAddress)) {
        d->setLastError(HostUrlInvalid);
        return false;
    }

    if (allowedSchemas == AllowedSchemas::AllowExternalRegistration && QtROServerFactory::instance()->isValid(hostAddress)) {
        qWarning() << qPrintable(objectName()) << "Overriding a valid QtRO url (" << hostAddress << ") with AllowExternalRegistration is not allowed.";
        d->setLastError(HostUrlInvalid);
        return false;
    }
    d->remoteObjectIo = new QRemoteObjectSourceIo(hostAddress, this);

    if (allowedSchemas == AllowedSchemas::BuiltInSchemasOnly && !d->remoteObjectIo->startListening()) {
        d->setLastError(ListenFailed);
        delete d->remoteObjectIo;
        d->remoteObjectIo = nullptr;
        return false;
    }

    //If we've given a name to the node, set it on the sourceIo as well
    if (!objectName().isEmpty())
        d->remoteObjectIo->setObjectName(objectName());
    //Since we don't know whether setHostUrl or setRegistryUrl/setRegistryHost will be called first,
    //break it into two pieces.  setHostUrl connects the RemoteObjectSourceIo->[add/remove]RemoteObjectSource to QRemoteObjectReplicaNode->[add/remove]RemoteObjectSource
    //setRegistry* calls appropriately connect RemoteObjecSourcetIo->[add/remove]RemoteObjectSource to the registry when it is created
    QObject::connect(d->remoteObjectIo, &QRemoteObjectSourceIo::remoteObjectAdded, this, &QRemoteObjectHostBase::remoteObjectAdded);
    QObject::connect(d->remoteObjectIo, &QRemoteObjectSourceIo::remoteObjectRemoved, this, &QRemoteObjectHostBase::remoteObjectRemoved);

    return true;
}

/*!
    \qmlproperty url Host::hostUrl

    The host address for the node.

    This is the address where source objects remoted by this node will reside.
*/

/*!
    \property QRemoteObjectHost::hostUrl
    \brief The host address for the node.

    This is the address where source objects remoted by this node will reside.
*/

/*!
    Returns the host address for the QRemoteObjectNode as a QUrl. If the Node
    is not a Host node, returns an empty QUrl.

    \sa setHostUrl()
*/
QUrl QRemoteObjectHost::hostUrl() const
{
    return QRemoteObjectHostBase::hostUrl();
}

/*!
    Sets the \a hostAddress for a host QRemoteObjectNode.

    Returns \c true if the Host address is set, otherwise \c false.

    The \a allowedSchemas parameter is only needed (and should be set to \l
    {QRemoteObjectHostBase::AllowExternalRegistration}
    {AllowExternalRegistration}) if the schema of the url should be used as an
    \l {External Schemas} {External Schema} by the registry.
*/
bool QRemoteObjectHost::setHostUrl(const QUrl &hostAddress, AllowedSchemas allowedSchemas)
{
    bool success = QRemoteObjectHostBase::setHostUrl(hostAddress, allowedSchemas);
    if (success)
        emit hostUrlChanged();
    return success;
}

/*!
    This method can be used to set the address of this Node to \a registryUrl
    (used for other Nodes to connect to this one), if the QUrl isn't set in the
    constructor. Since this Node becomes the Registry, calling this setter
    method causes this Node to use the url as the host address. All other
    Node's use the \l {QRemoteObjectNode::setRegistryUrl} method initiate a
    connection to the Registry.

    Returns \c true if the registry address is set, otherwise \c false.

    \sa QRemoteObjectRegistryHost(), QRemoteObjectNode::setRegistryUrl
*/
bool QRemoteObjectRegistryHost::setRegistryUrl(const QUrl &registryUrl)
{
    Q_D(QRemoteObjectRegistryHost);
    if (setHostUrl(registryUrl)) {
        if (!d->remoteObjectIo) {
            d->setLastError(ServerAlreadyCreated);
            return false;
        } else if (d->registry) {
            d->setLastError(RegistryAlreadyHosted);
            return false;
        }

        QRegistrySource *remoteObject = new QRegistrySource(this);
        enableRemoting(remoteObject);
        d->registryAddress = d->remoteObjectIo->serverAddress();
        d->registrySource = remoteObject;
        //Connect RemoteObjectSourceIo->remoteObject[Added/Removde] to the registry Slot
        QObject::connect(this, &QRemoteObjectRegistryHost::remoteObjectAdded, d->registrySource, &QRegistrySource::addSource);
        QObject::connect(this, &QRemoteObjectRegistryHost::remoteObjectRemoved, d->registrySource, &QRegistrySource::removeSource);
        QObject::connect(d->remoteObjectIo, &QRemoteObjectSourceIo::serverRemoved,d->registrySource, &QRegistrySource::removeServer);
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
    return d->lastError;
}

/*!
    \qmlproperty url Node::registryUrl

    The address of the \l {QRemoteObjectRegistry} {Registry} used by this node.

    This is an empty QUrl if there is no registry in use.
*/

/*!
    \property QRemoteObjectNode::registryUrl
    \brief The address of the \l {QRemoteObjectRegistry} {Registry} used by this node.

    This is an empty QUrl if there is no registry in use.
*/
QUrl QRemoteObjectNode::registryUrl() const
{
    Q_D(const QRemoteObjectNode);
    return d->registryAddress;
}

bool QRemoteObjectNode::setRegistryUrl(const QUrl &registryAddress)
{
    Q_D(QRemoteObjectNode);
    if (d->registry) {
        d->setLastError(RegistryAlreadyHosted);
        return false;
    }

    d->registryAddress = registryAddress;
    d->setRegistry(acquire<QRemoteObjectRegistry>());
    //Connect remoteObject[Added/Removed] to the registry Slot
    QObject::connect(this, &QRemoteObjectNode::remoteObjectAdded, d->registry, &QRemoteObjectRegistry::addSource);
    QObject::connect(this, &QRemoteObjectNode::remoteObjectRemoved, d->registry, &QRemoteObjectRegistry::removeSource);
    connectToNode(registryAddress);
    return true;
}

void QRemoteObjectNodePrivate::setRegistry(QRemoteObjectRegistry *reg)
{
    Q_Q(QRemoteObjectNode);
    registry = reg;
    reg->setParent(q);
    //Make sure when we get the registry initialized, we update our replicas
    QObject::connect(reg, &QRemoteObjectRegistry::initialized, q, [this]() {
        onRegistryInitialized();
    });
    //Make sure we handle new RemoteObjectSources on Registry...
    QObject::connect(reg, &QRemoteObjectRegistry::remoteObjectAdded,
                     q, [this](const QRemoteObjectSourceLocation &location) {
        onRemoteObjectSourceAdded(location);
    });
    QObject::connect(reg, &QRemoteObjectRegistry::remoteObjectRemoved,
                     q, [this](const QRemoteObjectSourceLocation &location) {
        onRemoteObjectSourceRemoved(location);
    });
}

QVariant QRemoteObjectNodePrivate::handlePointerToQObjectProperty(QConnectedReplicaImplementation *rep, int index, const QVariant &property)
{
    Q_Q(QRemoteObjectNode);
    using namespace QRemoteObjectPackets;

    QVariant retval;

    Q_ASSERT(property.canConvert<QRO_>());
    QRO_ childInfo = property.value<QRO_>();
    qROPrivDebug() << "QRO_:" << childInfo.name << replicas.contains(childInfo.name) << replicas.keys();
    if (childInfo.isNull) {
        // Either the source has changed the pointer and we need to update it, or the source pointer is a nullptr
        if (replicas.contains(childInfo.name))
            replicas.remove(childInfo.name);
        if (childInfo.type == ObjectType::CLASS)
            retval = QVariant::fromValue<QRemoteObjectDynamicReplica*>(nullptr);
        else
            retval = QVariant::fromValue<QAbstractItemModelReplica*>(nullptr);
        return retval;
    }

    const bool newReplica = !replicas.contains(childInfo.name) || rep->isInitialized();
    if (newReplica) {
        if (rep->isInitialized()) {
            auto childRep = qSharedPointerCast<QConnectedReplicaImplementation>(replicas.take(childInfo.name));
            if (childRep && !childRep->isShortCircuit()) {
                qCDebug(QT_REMOTEOBJECT) << "Checking if dynamic type should be added to dynamicTypeManager (type =" << childRep->m_metaObject->className() << ")";
                dynamicTypeManager.addFromMetaObject(childRep->m_metaObject);
            }
        }
        if (childInfo.type == ObjectType::CLASS)
            retval = QVariant::fromValue(q->acquireDynamic(childInfo.name));
        else
            retval = QVariant::fromValue(q->acquireModel(childInfo.name));
    } else //We are receiving the initial data for the QObject
        retval = rep->getProperty(index); //Use existing value so changed signal isn't emitted

    QSharedPointer<QConnectedReplicaImplementation> childRep = qSharedPointerCast<QConnectedReplicaImplementation>(replicas.value(childInfo.name).toStrongRef());
    if (childRep->connectionToSource.isNull())
        childRep->connectionToSource = rep->connectionToSource;
    QVariantList parameters;
    QDataStream ds(childInfo.parameters);
    if (childRep->needsDynamicInitialization()) {
        if (childInfo.classDefinition.isEmpty()) {
            auto typeName = childInfo.typeName;
            if (typeName == QLatin1String("QObject")) {
                // The sender would have included the class name if needed
                // So the acquire must have been templated, and we have the typeName
                typeName = QString::fromLatin1(rep->getProperty(index).typeName());
                if (typeName.endsWith(QLatin1String("Replica*")))
                    typeName.chop(8);
            }
            childRep->setDynamicMetaObject(dynamicTypeManager.metaObjectForType(typeName));
        } else {
            QDataStream in(childInfo.classDefinition);
            childRep->setDynamicMetaObject(dynamicTypeManager.addDynamicType(rep->connectionToSource, in));
        }
        if (!childInfo.parameters.isEmpty())
            ds >> parameters;
        handlePointerToQObjectProperties(childRep.data(), parameters);
        childRep->setDynamicProperties(parameters);
    } else {
        if (!childInfo.parameters.isEmpty())
            ds >> parameters;
        handlePointerToQObjectProperties(childRep.data(), parameters);
        childRep->initialize(parameters);
    }

    return retval;
}

void QRemoteObjectNodePrivate::handlePointerToQObjectProperties(QConnectedReplicaImplementation *rep, QVariantList &properties)
{
    for (const int index : rep->childIndices())
        properties[index] = handlePointerToQObjectProperty(rep, index, properties.at(index));
}

/*!
    Blocks until this Node's \l Registry is initialized or \a timeout (in
    milliseconds) expires. Returns \c true if the \l Registry is successfully
    initialized upon return, or \c false otherwise.
*/
bool QRemoteObjectNode::waitForRegistry(int timeout)
{
    Q_D(QRemoteObjectNode);
    if (!d->registry) {
        qCWarning(QT_REMOTEOBJECT) << qPrintable(objectName()) << "waitForRegistry() error: No valid registry url set";
        return false;
    }

    return d->registry->waitForSource(timeout);
}

/*!
    Connects a client node to the host node at \a address.

    Connections will remain valid until the host node is deleted or no longer
    accessible over a network.

    Once a client is connected to a host, valid Replicas can then be acquired
    if the corresponding Source is being remoted.

    Return \c true on success, \c false otherwise (usually an unrecognized url,
    or connecting to already connected address).
*/
bool QRemoteObjectNode::connectToNode(const QUrl &address)
{
    Q_D(QRemoteObjectNode);
    if (!d->initConnection(address)) {
        d->setLastError(RegistryNotAcquired);
        return false;
    }
    return true;
}

/*!
    \since 5.12

    In order to \l QRemoteObjectNode::acquire() \l Replica objects over \l
    {External QIODevices}, Qt Remote Objects needs access to the communications
    channel (a \l QIODevice) between the respective nodes. It is the
    addClientSideConnection() call that enables this, taking the \a ioDevice as
    input. Any acquire() call made without calling addClientSideConnection will
    still work, but the Node will not be able to initialize the \l Replica
    without being provided the connection to the Host node.

    \sa {QRemoteObjectHostBase::addHostSideConnection}
*/
void QRemoteObjectNode::addClientSideConnection(QIODevice *ioDevice)
{
    Q_D(QRemoteObjectNode);
    ExternalIoDevice *device = new ExternalIoDevice(ioDevice, this);
    connect(device, &IoDeviceBase::readyRead, this, [d, device]() {
        d->onClientRead(device);
    });
    if (device->bytesAvailable())
        d->onClientRead(device);
}

/*!
    \fn void QRemoteObjectNode::remoteObjectAdded(const QRemoteObjectSourceLocation &loc)

    This signal is emitted whenever a new \l {Source} object is added to
    the Registry. The signal will not be emitted if there is no Registry set
    (i.e., Sources over connections made via connectToNode directly). The \a
    loc parameter contains the information about the added Source, including
    name, type and the QUrl of the hosting Node.

    \sa remoteObjectRemoved(), instances()
*/

/*!
    \fn void QRemoteObjectNode::remoteObjectRemoved(const QRemoteObjectSourceLocation &loc)

    This signal is emitted whenever there is a known \l {Source} object is
    removed from the Registry. The signal will not be emitted if there is no
    Registry set (i.e., Sources over connections made via connectToNode
    directly). The \a loc parameter contains the information about the removed
    Source, including name, type and the QUrl of the hosting Node.

    \sa remoteObjectAdded, instances
*/

/*!
    \fn QStringList QRemoteObjectNode::instances() const

    This templated function (taking a \l repc generated type as the template parameter) will
    return the list of names of every instance of that type on the Remote
    Objects network. For example, if you have a Shape class defined in a .rep file,
    and Circle and Square classes inherit from the Source definition, they can
    be shared on the Remote Objects network using \l {QRemoteObjectHostBase::enableRemoting} {enableRemoting}.
    \code
        Square square;
        Circle circle;
        myHost.enableRemoting(&square, "Square");
        myHost.enableRemoting(&circle, "Circle");
    \endcode
    Then instance can be used to find the available instances of Shape.
    \code
        QStringList instances = clientNode.instances<Shape>();
        // will return a QStringList containing "Circle" and "Square"
        auto instance1 = clientNode.acquire<Shape>("Circle");
        auto instance2 = clientNode.acquire<Shape>("Square");
        ...
    \endcode
*/

/*!
    \overload instances()

    This convenience function provides the same result as the templated
    version, but takes the name of the \l {Source} class as a parameter (\a
    typeName) rather than deriving it from the class type.
*/
QStringList QRemoteObjectNode::instances(const QString &typeName) const
{
    Q_D(const QRemoteObjectNode);
    QStringList names;
    for (auto it = d->connectedSources.cbegin(), end = d->connectedSources.cend(); it != end; ++it) {
        if (it.value().typeName == typeName) {
            names << it.key();
        }
    }
    return names;
}

/*!
    \keyword dynamic acquire
    Returns a QRemoteObjectDynamicReplica of the Source \a name.
*/
QRemoteObjectDynamicReplica *QRemoteObjectNode::acquireDynamic(const QString &name)
{
    return new QRemoteObjectDynamicReplica(this, name);
}

/*!
    \qmlmethod void Host::enableRemoting(object object, string name)
    Enables a host node to dynamically provide remote access to the QObject \a
    object. Client nodes connected to the node hosting this object may obtain
    Replicas of this Source.

    The \a name defines the lookup-name under which the QObject can be acquired
    using \l QRemoteObjectNode::acquire() . If not explicitly set then the name
    given in the QCLASSINFO_REMOTEOBJECT_TYPE will be used. If no such macro
    was defined for the QObject then the \l QObject::objectName() is used.

    Returns \c false if the current node is a client node, or if the QObject is already
    registered to be remoted, and \c true if remoting is successfully enabled
    for the dynamic QObject.

    \sa disableRemoting()
*/

/*!
    Enables a host node to dynamically provide remote access to the QObject \a
    object. Client nodes connected to the node
    hosting this object may obtain Replicas of this Source.

    The \a name defines the lookup-name under which the QObject can be acquired
    using \l QRemoteObjectNode::acquire() . If not explicitly set then the name
    given in the QCLASSINFO_REMOTEOBJECT_TYPE will be used. If no such macro
    was defined for the QObject then the \l QObject::objectName() is used.

    Returns \c false if the current node is a client node, or if the QObject is already
    registered to be remoted, and \c true if remoting is successfully enabled
    for the dynamic QObject.

    \sa disableRemoting()
*/
bool QRemoteObjectHostBase::enableRemoting(QObject *object, const QString &name)
{
    Q_D(QRemoteObjectHostBase);
    if (!d->remoteObjectIo) {
        d->setLastError(OperationNotValidOnClientNode);
        return false;
    }

    const QMetaObject *meta = object->metaObject();
    QString _name = name;
    QString typeName = getTypeNameAndMetaobjectFromClassInfo(meta);
    if (typeName.isEmpty()) { //This is a passed in QObject, use its API
        if (_name.isEmpty()) {
            _name = object->objectName();
            if (_name.isEmpty()) {
                d->setLastError(MissingObjectName);
                qCWarning(QT_REMOTEOBJECT) << qPrintable(objectName()) << "enableRemoting() Error: Unable to Replicate an object that does not have objectName() set.";
                return false;
            }
        }
    } else if (_name.isEmpty())
        _name = typeName;
    return d->remoteObjectIo->enableRemoting(object, meta, _name, typeName);
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
    QObject *adapter = QAbstractItemModelSourceAdapter::staticMetaObject.newInstance(Q_ARG(QAbstractItemModel*, model),
                                                                                     Q_ARG(QItemSelectionModel*, selectionModel),
                                                                                     Q_ARG(QVector<int>, roles));
    QAbstractItemAdapterSourceAPI<QAbstractItemModel, QAbstractItemModelSourceAdapter> *api =
        new QAbstractItemAdapterSourceAPI<QAbstractItemModel, QAbstractItemModelSourceAdapter>(name);
    if (!this->objectName().isEmpty())
        adapter->setObjectName(this->objectName().append(QLatin1String("Adapter")));
    return enableRemoting(model, api, adapter);
}

/*!
    \fn template <template <typename> class ApiDefinition, typename ObjectType> bool QRemoteObjectHostBase::enableRemoting(ObjectType *object)

    This templated function overload enables a host node to provide remote
    access to a QObject \a object with a specified (and compile-time checked)
    interface. Client nodes connected to the node hosting this object may
    obtain Replicas of this Source.

    This is best illustrated by example:
    \code
        #include "rep_TimeModel_source.h"
        MinuteTimer timer;
        hostNode.enableRemoting<MinuteTimerSourceAPI>(&timer);
    \endcode

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
    \internal
    Enables a host node to provide remote access to a QObject \a object
    with the API defined by \a api. Client nodes connected to the node
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
    \qmlmethod void Host::disableRemoting(object remoteObject)
    Disables remote access for the QObject \a remoteObject. Returns \c false if
    the current node is a client node or if the \a remoteObject is not
    registered, and returns \c true if remoting is successfully disabled for
    the Source object.

    \warning Replicas of this object will no longer be valid after calling this method.

    \sa enableRemoting()
*/

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
        d->setLastError(OperationNotValidOnClientNode);
        return false;
    }

    if (!d->remoteObjectIo->disableRemoting(remoteObject)) {
        d->setLastError(SourceNotRegistered);
        return false;
    }

    return true;
}

/*!
    \since 5.12

    In order to \l QRemoteObjectHost::enableRemoting() \l Source objects over
    \l {External QIODevices}, Qt Remote Objects needs access to the
    communications channel (a \l QIODevice) between the respective nodes. It is
    the addHostSideConnection() call that enables this on the \l Source side,
    taking the \a ioDevice as input. Any enableRemoting() call will still work
    without calling addHostSideConnection, but the Node will not be able to
    share the \l Source objects without being provided the connection to
    the Replica node. Before calling this function you must call
    \l {QRemoteObjectHost::}{setHostUrl}() with a unique URL and
    \l {QRemoteObjectHost::}{AllowExternalRegistration}.

    \sa addClientSideConnection
*/
void QRemoteObjectHostBase::addHostSideConnection(QIODevice *ioDevice)
{
    Q_D(QRemoteObjectHostBase);
    if (!d->remoteObjectIo)
        d->remoteObjectIo = new QRemoteObjectSourceIo(this);
    ExternalIoDevice *device = new ExternalIoDevice(ioDevice, this);
    return d->remoteObjectIo->newConnection(device);
}

/*!
 Returns a pointer to a Replica which is specifically derived from \l
 QAbstractItemModel. The \a name provided must match the name used with the
 matching \l {QRemoteObjectHostBase::}{enableRemoting} that put
 the Model on the network. The returned model will be empty until it is
 initialized with the \l Source.
 */
QAbstractItemModelReplica *QRemoteObjectNode::acquireModel(const QString &name, QtRemoteObjects::InitialAction action, const QVector<int> &rolesHint)
{
    QAbstractItemModelReplicaImplementation *rep = acquire<QAbstractItemModelReplicaImplementation>(name);
    return new QAbstractItemModelReplica(rep, action, rolesHint);
}

QRemoteObjectHostBasePrivate::QRemoteObjectHostBasePrivate()
    : QRemoteObjectNodePrivate()
    , remoteObjectIo(nullptr)
{ }

QRemoteObjectHostBasePrivate::~QRemoteObjectHostBasePrivate()
{ }

QRemoteObjectHostPrivate::QRemoteObjectHostPrivate()
    : QRemoteObjectHostBasePrivate()
{ }

QRemoteObjectHostPrivate::~QRemoteObjectHostPrivate()
{ }

QRemoteObjectRegistryHostPrivate::QRemoteObjectRegistryHostPrivate()
    : QRemoteObjectHostBasePrivate()
    , registrySource(nullptr)
{ }

QRemoteObjectRegistryHostPrivate::~QRemoteObjectRegistryHostPrivate()
{ }

ProxyInfo::ProxyInfo(QRemoteObjectNode *node, QRemoteObjectHostBase *parent,
                     QRemoteObjectHostBase::RemoteObjectNameFilter filter)
    : QObject(parent)
    , proxyNode(node)
    , parentNode(parent)
    , proxyFilter(filter)
{
    const auto registry = node->registry();
    proxyNode->setObjectName(QString::fromLatin1("_ProxyNode"));

    connect(registry, &QRemoteObjectRegistry::remoteObjectAdded, this,
            [this](const QRemoteObjectSourceLocation &entry)
    {
        this->proxyObject(entry, ProxyDirection::Forward);
    });
    connect(registry, &QRemoteObjectRegistry::remoteObjectRemoved, this,
            &ProxyInfo::unproxyObject);
    connect(registry, &QRemoteObjectRegistry::initialized, this, [registry, this]() {
        QRemoteObjectSourceLocations locations = registry->sourceLocations();
        QRemoteObjectSourceLocations::const_iterator i = locations.constBegin();
        while (i != locations.constEnd()) {
            proxyObject(QRemoteObjectSourceLocation(i.key(), i.value()));
            ++i;
        }
    });

    connect(registry, &QRemoteObjectRegistry::stateChanged, this,
            [this](QRemoteObjectRegistry::State state, QRemoteObjectRegistry::State /*oldState*/) {
        if (state != QRemoteObjectRegistry::Suspect)
            return;
        // unproxy all objects
        for (ProxyReplicaInfo* info : proxiedReplicas)
            disableAndDeleteObject(info);
        proxiedReplicas.clear();
    });
}

ProxyInfo::~ProxyInfo() {
    for (ProxyReplicaInfo* info : proxiedReplicas)
        delete info;
}

bool ProxyInfo::setReverseProxy(QRemoteObjectHostBase::RemoteObjectNameFilter filter)
{
    const auto registry = proxyNode->registry();
    this->reverseFilter = filter;

    connect(registry, &QRemoteObjectRegistry::remoteObjectAdded, this,
            [this](const QRemoteObjectSourceLocation &entry)
    {
        this->proxyObject(entry, ProxyDirection::Reverse);
    });
    connect(registry, &QRemoteObjectRegistry::remoteObjectRemoved, this,
            &ProxyInfo::unproxyObject);
    connect(registry, &QRemoteObjectRegistry::initialized, this, [registry, this]() {
        QRemoteObjectSourceLocations locations = registry->sourceLocations();
        QRemoteObjectSourceLocations::const_iterator i = locations.constBegin();
        while (i != locations.constEnd()) {
            proxyObject(QRemoteObjectSourceLocation(i.key(), i.value()), ProxyDirection::Reverse);
            ++i;
        }
    });

    return true;
}

void ProxyInfo::proxyObject(const QRemoteObjectSourceLocation &entry, ProxyDirection direction)
{
    const QString name = entry.first;
    Q_ASSERT(!proxiedReplicas.contains(name));
    const QString typeName = entry.second.typeName;

    if (direction == ProxyDirection::Forward) {
        if (!proxyFilter(name, typeName))
            return;

        qCDebug(QT_REMOTEOBJECT) << "Starting proxy for" << name << "from" << entry.second.hostUrl;

        if (entry.second.typeName == QAIMADAPTER()) {
            QAbstractItemModelReplica *rep = proxyNode->acquireModel(name);
            proxiedReplicas.insert(name, new ProxyReplicaInfo{rep, direction});
            connect(rep, &QAbstractItemModelReplica::initialized, this,
                    [rep, name, this]() { this->parentNode->enableRemoting(rep, name, QVector<int>()); });
        } else {
            QRemoteObjectDynamicReplica *rep = proxyNode->acquireDynamic(name);
            proxiedReplicas.insert(name, new ProxyReplicaInfo{rep, direction});
            connect(rep, &QRemoteObjectDynamicReplica::initialized, this,
                    [rep, name, this]() { this->parentNode->enableRemoting(rep, name); });
        }
    } else {
        if (!reverseFilter(name, typeName))
            return;

        qCDebug(QT_REMOTEOBJECT) << "Starting reverse proxy for" << name << "from" << entry.second.hostUrl;

        if (entry.second.typeName == QAIMADAPTER()) {
            QAbstractItemModelReplica *rep = this->parentNode->acquireModel(name);
            proxiedReplicas.insert(name, new ProxyReplicaInfo{rep, direction});
            connect(rep, &QAbstractItemModelReplica::initialized, this,
                    [rep, name, this]()
            {
                QRemoteObjectHostBase *host = qobject_cast<QRemoteObjectHostBase *>(this->proxyNode);
                Q_ASSERT(host);
                host->enableRemoting(rep, name, QVector<int>());
            });
        } else {
            QRemoteObjectDynamicReplica *rep = this->parentNode->acquireDynamic(name);
            proxiedReplicas.insert(name, new ProxyReplicaInfo{rep, direction});
            connect(rep, &QRemoteObjectDynamicReplica::initialized, this,
                    [rep, name, this]()
            {
                QRemoteObjectHostBase *host = qobject_cast<QRemoteObjectHostBase *>(this->proxyNode);
                Q_ASSERT(host);
                host->enableRemoting(rep, name);
            });
        }
    }

}

void ProxyInfo::unproxyObject(const QRemoteObjectSourceLocation &entry)
{
    const QString name = entry.first;

    if (proxiedReplicas.contains(name)) {
        qCDebug(QT_REMOTEOBJECT)  << "Stopping proxy for" << name;
        auto const info = proxiedReplicas.take(name);
        disableAndDeleteObject(info);
    }
}

void ProxyInfo::disableAndDeleteObject(ProxyReplicaInfo* info)
{
    if (info->direction == ProxyDirection::Forward)
        this->parentNode->disableRemoting(info->replica);
    else {
        QRemoteObjectHostBase *host = qobject_cast<QRemoteObjectHostBase *>(this->proxyNode);
        Q_ASSERT(host);
        host->disableRemoting(info->replica);
    }
    delete info;
}


QT_END_NAMESPACE

#include "moc_qremoteobjectnode.cpp"
