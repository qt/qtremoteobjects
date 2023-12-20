// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qremoteobjectreplica.h"
#include "qremoteobjectreplica_p.h"

#include "qremoteobjectnode.h"
#include "qremoteobjectnode_p.h"
#include "qremoteobjectdynamicreplica.h"
#include "qremoteobjectpacket_p.h"
#include "qremoteobjectpendingcall_p.h"
#include "qconnectionfactories_p.h"
#include "qremoteobjectsource_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qvariant.h>
#include <QtCore/qthread.h>

#include <limits>

QT_BEGIN_NAMESPACE

using namespace QRemoteObjectPackets;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wtautological-compare")

#if !defined(Q_OS_WIN) && !defined(Q_OS_INTEGRITY)
Q_STATIC_ASSERT_X(&QRemoteObjectReplica::staticMetaObject == &QRemoteObjectDynamicReplica::staticMetaObject,
                  "m_signalOffset initializer expects QRemoteObjectDynamicReplica to not have a unique staticMetaObject");
#endif

QT_WARNING_POP

// If QRemoteObjectDynamicReplica ever gets its own staticMetaObject, some commented out code will need to be
// used.  It was changed to avoid a Coverity complaint.  We use the above static assert to detect if this changes
// in the future.  See FIX #1, #2, #3 in this file.

QRemoteObjectReplicaImplementation::QRemoteObjectReplicaImplementation(const QString &name, const QMetaObject *meta, QRemoteObjectNode *_node)
    : QObject(nullptr), m_objectName(name), m_metaObject(meta), m_numSignals(0), m_methodOffset(0)
    // Uncomment the following two lines if QRemoteObjectDynamicReplica gets a unique staticMetaObject (FIX #1, #2)
    //, m_signalOffset(meta ? QRemoteObjectReplica::staticMetaObject.methodCount() : QRemoteObjectDynamicReplica::staticMetaObject.methodCount())
    //, m_propertyOffset(meta ? QRemoteObjectReplica::staticMetaObject.propertyCount() : QRemoteObjectDynamicReplica::staticMetaObject.propertyCount())
    , m_signalOffset(QRemoteObjectReplica::staticMetaObject.methodCount())
    , m_propertyOffset(QRemoteObjectReplica::staticMetaObject.propertyCount())
    , m_node(_node)
    , m_objectSignature(QtPrivate::qtro_classinfo_signature(m_metaObject))
    , m_state(meta ? QRemoteObjectReplica::Default : QRemoteObjectReplica::Uninitialized)
{
}

QRemoteObjectReplicaImplementation::~QRemoteObjectReplicaImplementation()
{
}

QConnectedReplicaImplementation::QConnectedReplicaImplementation(const QString &name, const QMetaObject *meta, QRemoteObjectNode *node)
    : QRemoteObjectReplicaImplementation(name, meta, node), connectionToSource(nullptr)
{
    m_heartbeatTimer.setTimerType(Qt::CoarseTimer);
    m_heartbeatTimer.setSingleShot(true);
    m_heartbeatTimer.setInterval(node->heartbeatInterval());

    connect(node, &QRemoteObjectNode::heartbeatIntervalChanged, this, [this](int interval) {
        m_heartbeatTimer.stop();
        m_heartbeatTimer.setInterval(interval);
        if (interval)
            m_heartbeatTimer.start();
    });
    connect(&m_heartbeatTimer, &QTimer::timeout, this, [this] {
        // TODO: Revisit if a baseclass method can be used to avoid specialized cast
        // conditional logic.

        if (m_pendingCalls.contains(0)) {
            m_pendingCalls.take(0);
            // The source didn't respond in time, disconnect the connection
            if (connectionToSource) {
                auto clientIo = qobject_cast<QtROClientIoDevice *>(connectionToSource);
                if (clientIo)
                    clientIo->disconnectFromServer();
                else
                    connectionToSource->close();
            }
        } else {
            if (connectionToSource.isNull()) {
                qCDebug(QT_REMOTEOBJECT) << "Ignoring heartbeat as there is no source connected.";
                return;
            }
            connectionToSource->d_func()->m_codec->serializePingPacket(m_objectName);
            if (sendCommandWithReply(0).d->serialId == -1) {
                m_heartbeatTimer.stop();
                auto clientIo = qobject_cast<QtROClientIoDevice *>(connectionToSource);
                if (clientIo)
                    clientIo->disconnectFromServer();
                else
                    connectionToSource->close();
            }
        }
    });

    if (!meta)
        return;

    auto offsetMeta = m_metaObject;
    QtRemoteObjects::getTypeNameAndMetaobjectFromClassInfo(offsetMeta);
    for (int index = offsetMeta->propertyOffset(); index < offsetMeta->propertyCount(); ++index) {
        const QMetaProperty property = offsetMeta->property(index);
        if (property.metaType().flags().testFlag(QMetaType::PointerToQObject))
            m_childIndices << index - offsetMeta->propertyOffset();
    }
}

QConnectedReplicaImplementation::~QConnectedReplicaImplementation()
{
    if (!connectionToSource.isNull()) {
        qCDebug(QT_REMOTEOBJECT) << "Replica deleted: sending RemoveObject to RemoteObjectSource" << m_objectName;
        connectionToSource->d_func()->m_codec->serializeRemoveObjectPacket(m_objectName);
        sendCommand();
    }
    for (auto prop : m_propertyStorage) {
        if (prop.canConvert<QObject*>()) {
            if (auto o = prop.value<QObject*>())
                o->deleteLater();
        }
    }
}

bool QRemoteObjectReplicaImplementation::needsDynamicInitialization() const
{
    return m_metaObject == nullptr;
}

void QRemoteObjectReplicaImplementation::setState(QRemoteObjectReplica::State state)
{
    if (m_state.loadAcquire() != QRemoteObjectReplica::Suspect && m_state.loadAcquire() >= state)
        return;

    int oldState = m_state.loadAcquire();
    m_state.storeRelease(state);

    // We should emit initialized before emitting any changed signals in case connections are made in a
    // Slot responding to initialized/validChanged.
    if (m_state.loadAcquire() == QRemoteObjectReplica::Valid) {
        // we're initialized now, emit signal
        emitInitialized();
    }

    const static int stateChangedIndex = QRemoteObjectReplica::staticMetaObject.indexOfMethod("stateChanged(State,State)");
    Q_ASSERT(stateChangedIndex != -1);
    void *args[] = {nullptr, &state, &oldState};
    QMetaObject::activate(this, metaObject(), stateChangedIndex, args);
}

void QRemoteObjectReplicaImplementation::emitNotified()
{
    const static int notifiedIndex = QRemoteObjectReplica::staticMetaObject.indexOfMethod("notified()");
    Q_ASSERT(notifiedIndex != -1);
    void *args[] = {nullptr};
    QMetaObject::activate(this, metaObject(), notifiedIndex, args);
}

bool QConnectedReplicaImplementation::sendCommand()
{
    Q_ASSERT(connectionToSource);
    if (!connectionToSource->isOpen())
        return false;

    connectionToSource->d_func()->m_codec->send(connectionToSource);
    if (m_heartbeatTimer.interval())
        m_heartbeatTimer.start();
    return true;
}

QList<int> QConnectedReplicaImplementation::childIndices() const
{
    return m_childIndices;
}

void QConnectedReplicaImplementation::initialize(QVariantList &&values)
{
    qCDebug(QT_REMOTEOBJECT) << "initialize()" << m_propertyStorage.size();
    const int nParam = int(values.size());
    QVarLengthArray<int> changedProperties(nParam);
    const int offset = m_propertyOffset;
    for (int i = 0; i < nParam; ++i) {
        qCDebug(QT_REMOTEOBJECT) << "  in loop" << i << m_propertyStorage.size();
        changedProperties[i] = -1;
        if (m_propertyStorage[i] != values.at(i)) {
            const QMetaProperty property = m_metaObject->property(i+offset);
            m_propertyStorage[i] = QRemoteObjectPackets::decodeVariant(std::move(values[i]), property.metaType());
            changedProperties[i] = i;
        }
        qCDebug(QT_REMOTEOBJECT) << "SETPROPERTY" << i << m_metaObject->property(i+offset).name()
                                 << m_propertyStorage[i].typeName()
                                 << m_propertyStorage[i].toString();
    }

    Q_ASSERT(m_state.loadAcquire() < QRemoteObjectReplica::Valid || m_state.loadAcquire() == QRemoteObjectReplica::Suspect);
    setState(QRemoteObjectReplica::Valid);

    void *args[] = {nullptr, nullptr};
    for (int i = 0; i < nParam; ++i) {
        if (changedProperties[i] < 0)
            continue;
        const int notifyIndex = m_metaObject->property(changedProperties[i]+offset).notifySignalIndex();
        if (notifyIndex < 0)
            continue;
        qCDebug(QT_REMOTEOBJECT) << " Before activate" << notifyIndex << m_metaObject->property(notifyIndex).name();
        args[1] = m_propertyStorage[i].data();
        QMetaObject::activate(this, metaObject(), notifyIndex, args);
    }
    emitNotified();

    qCDebug(QT_REMOTEOBJECT) << "isSet = true for" << m_objectName;
    if (node()->heartbeatInterval())
        m_heartbeatTimer.start();
}

void QRemoteObjectReplicaImplementation::emitInitialized()
{
    const static int initializedIndex = QRemoteObjectReplica::staticMetaObject.indexOfMethod("initialized()");
    Q_ASSERT(initializedIndex != -1);
    void *noArgs[] = {nullptr};
    QMetaObject::activate(this, metaObject(), initializedIndex, noArgs);
}

/*!
    \internal
*/
void QRemoteObjectReplica::persistProperties(const QString &repName, const QByteArray &repSig, const QVariantList &props) const
{
    if (!node()) {
        qWarning("Tried calling persistProperties on a replica (%s) that hasn't been initialized with a node", qPrintable(repName));
        return;
    }
    node()->persistProperties(repName, repSig, props);
}

/*!
    \internal
*/
QVariantList QRemoteObjectReplica::retrieveProperties(const QString &repName, const QByteArray &repSig) const
{
    if (!node()) {
        qWarning("Tried calling retrieveProperties on a replica (%s) that hasn't been initialized with a node", qPrintable(repName));
        return QVariantList();
    }
    return node()->retrieveProperties(repName, repSig);
}

void QRemoteObjectReplicaImplementation::setDynamicMetaObject(const QMetaObject *meta)
{
    Q_ASSERT(!m_metaObject);

    m_metaObject = meta;
}

void QConnectedReplicaImplementation::setDynamicMetaObject(const QMetaObject *meta)
{
    QRemoteObjectReplicaImplementation::setDynamicMetaObject(meta);

    for (int index = m_metaObject->propertyOffset(); index < m_metaObject->propertyCount(); ++index) {
        const QMetaProperty property = m_metaObject->property(index);
        if (property.metaType().flags().testFlag(QMetaType::PointerToQObject))
            m_childIndices << index - m_metaObject->propertyOffset();
    }
}

void QRemoteObjectReplicaImplementation::setDynamicProperties(QVariantList &&values)
{
    const int offset = m_propertyOffset;
    int propertyIndex = -1;
    for (auto &prop : values) {
        propertyIndex++;
        const QMetaProperty property = m_metaObject->property(propertyIndex+offset);
        prop = QRemoteObjectPackets::decodeVariant(std::move(prop), property.metaType());
    }
    //rely on order of properties;
    setProperties(std::move(values));
}

void QConnectedReplicaImplementation::setDynamicProperties(QVariantList &&values)
{
    QRemoteObjectReplicaImplementation::setDynamicProperties(std::move(values));
    for (QRemoteObjectReplica *obj : std::exchange(m_parentsNeedingConnect, {}))
        configurePrivate(obj);

    Q_ASSERT(m_state.loadAcquire() < QRemoteObjectReplica::Valid);
    setState(QRemoteObjectReplica::Valid);

    void *args[] = {nullptr, nullptr};
    for (int index = m_metaObject->propertyOffset(); index < m_metaObject->propertyCount(); ++index) {
        const QMetaProperty mp = m_metaObject->property(index);
        if (mp.hasNotifySignal()) {
            qCDebug(QT_REMOTEOBJECT) << " Before activate" << index << m_metaObject->property(index).name();
            args[1] = this->m_propertyStorage[index-m_propertyOffset].data();
            QMetaObject::activate(this, metaObject(), mp.notifySignalIndex(), args);
        }
    }
    emitNotified();

    qCDebug(QT_REMOTEOBJECT) << "isSet = true for" << m_objectName;
}

bool QConnectedReplicaImplementation::isInitialized() const
{
    return  m_state.loadAcquire() > QRemoteObjectReplica::Default && m_state.loadAcquire() != QRemoteObjectReplica::SignatureMismatch;
}

bool QConnectedReplicaImplementation::waitForSource(int timeout)
{
    switch (state()) {
    case QRemoteObjectReplica::State::Valid:
        return true;
    case QRemoteObjectReplica::State::SignatureMismatch:
        return false;
    default:
        break;
    }

    const static int stateChangedIndex = QRemoteObjectReplica::staticMetaObject.indexOfMethod("stateChanged(State,State)");
    Q_ASSERT(stateChangedIndex != -1);

    QEventLoop loop;
    QMetaObject::connect(this, stateChangedIndex,
                         &loop, QEventLoop::staticMetaObject.indexOfMethod("quit()"),
                         Qt::DirectConnection, nullptr);

    QTimer t; // NB: Related to QTBUG-94570 - don't use QTimer::singleShot here.
    if (timeout >= 0) {
        t.setSingleShot(true);
        connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
        t.start(timeout);
    }

    // enter the event loop and wait for a reply
    loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);

    return state() == QRemoteObjectReplica::State::Valid;
}

void QConnectedReplicaImplementation::_q_send(QMetaObject::Call call, int index, const QVariantList &args)
{
    static const bool debugArgs = qEnvironmentVariableIsSet("QT_REMOTEOBJECT_DEBUG_ARGUMENTS");

    Q_ASSERT(call == QMetaObject::InvokeMetaMethod || call == QMetaObject::WriteProperty);
    if (connectionToSource.isNull()) {
        qCWarning(QT_REMOTEOBJECT) << "connectionToSource is null";
        return;
    }

    if (call == QMetaObject::InvokeMetaMethod) {
        if (debugArgs) {
            qCDebug(QT_REMOTEOBJECT) << "Send" << call << this->m_metaObject->method(index).name() << index << args << connectionToSource;
        } else {
            qCDebug(QT_REMOTEOBJECT) << "Send" << call << this->m_metaObject->method(index).name() << index << connectionToSource;
        }
        if (index < m_methodOffset) //index - m_methodOffset < 0 is invalid, and can't be resolved on the Source side
            qCWarning(QT_REMOTEOBJECT) << "Skipping invalid method invocation.  Index not found:" << index << "( offset =" << m_methodOffset << ") object:" << m_objectName << this->m_metaObject->method(index).name();
        else {
            connectionToSource->d_func()->m_codec->serializeInvokePacket(m_objectName, call, index - m_methodOffset, args);
            sendCommand();
        }
    } else {
        qCDebug(QT_REMOTEOBJECT) << "Send" << call << this->m_metaObject->property(index).name() << index << args << connectionToSource;
        if (index < m_propertyOffset) //index - m_propertyOffset < 0 is invalid, and can't be resolved on the Source side
            qCWarning(QT_REMOTEOBJECT) << "Skipping invalid property invocation.  Index not found:" << index << "( offset =" << m_propertyOffset << ") object:" << m_objectName << this->m_metaObject->property(index).name();
        else {
            connectionToSource->d_func()->m_codec->serializeInvokePacket(m_objectName, call, index - m_propertyOffset, args);
            sendCommand();
        }
    }
}

QRemoteObjectPendingCall QConnectedReplicaImplementation::_q_sendWithReply(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_ASSERT(call == QMetaObject::InvokeMetaMethod);
    if (connectionToSource.isNull()) {
        qCWarning(QT_REMOTEOBJECT) << "connectionToSource is null";
        return QRemoteObjectPendingCall();
    }

    qCDebug(QT_REMOTEOBJECT) << "Send" << call << this->m_metaObject->method(index).name() << index << args << connectionToSource;
    int serialId = (m_curSerialId == std::numeric_limits<int>::max() ? 1 : m_curSerialId++);
    connectionToSource->d_func()->m_codec->serializeInvokePacket(m_objectName, call, index - m_methodOffset, args, serialId);
    return sendCommandWithReply(serialId);
}

QRemoteObjectPendingCall QConnectedReplicaImplementation::sendCommandWithReply(int serialId)
{
    bool success = sendCommand();
    if (!success) {
        return QRemoteObjectPendingCall(); // invalid
    }

    qCDebug(QT_REMOTEOBJECT) << "Sent InvokePacket with serial id:" << serialId;
    QRemoteObjectPendingCall pendingCall(new QRemoteObjectPendingCallData(serialId, this));
    Q_ASSERT(!m_pendingCalls.contains(serialId));
    m_pendingCalls[serialId] = pendingCall;
    return pendingCall;
}

void QConnectedReplicaImplementation::notifyAboutReply(int ackedSerialId, const QVariant &value)
{
    QRemoteObjectPendingCall call = m_pendingCalls.take(ackedSerialId);
    if (ackedSerialId == 0) {
        m_heartbeatTimer.stop();
        if (m_heartbeatTimer.interval())
            m_heartbeatTimer.start();
        return;
    }

    QMutexLocker mutex(&call.d->mutex);

    // clear error flag
    call.d->error = QRemoteObjectPendingCall::NoError;
    call.d->returnValue = value;

    // notify watchers if needed
    if (call.d->watcherHelper)
        call.d->watcherHelper->emitSignals();
}

bool QConnectedReplicaImplementation::waitForFinished(const QRemoteObjectPendingCall& call, int timeout)
{
    if (!call.d->watcherHelper)
        call.d->watcherHelper.reset(new QRemoteObjectPendingCallWatcherHelper);

    call.d->mutex.unlock();

    QEventLoop loop;
    loop.connect(call.d->watcherHelper.data(), &QRemoteObjectPendingCallWatcherHelper::finished,
                 &loop, &QEventLoop::quit);

    QTimer t; // NB: Related to QTBUG-94570 - don't use QTimer::singleShot here.
    if (timeout >= 0) {
        t.setSingleShot(true);
        connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
        t.start(timeout);
    }

    // enter the event loop and wait for a reply
    loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);

    call.d->mutex.lock();

    return call.d->error != QRemoteObjectPendingCall::InvalidMessage;
}

const QVariant QConnectedReplicaImplementation::getProperty(int i) const
{
    Q_ASSERT_X(i >= 0 && i < m_propertyStorage.size(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(i).arg(m_propertyStorage.size())));
    return m_propertyStorage[i];
}

void QConnectedReplicaImplementation::setProperties(QVariantList &&properties)
{
    Q_ASSERT(m_propertyStorage.isEmpty());
    m_propertyStorage.reserve(properties.size());
    m_propertyStorage = std::move(properties);
}

void QConnectedReplicaImplementation::setProperty(int i, const QVariant &prop)
{
    m_propertyStorage[i] = prop;
}

void QConnectedReplicaImplementation::setConnection(QtROIoDeviceBase *conn)
{
    if (connectionToSource.isNull()) {
        connectionToSource = conn;
        qCDebug(QT_REMOTEOBJECT) << "setConnection started" << conn << m_objectName;
    }
    requestRemoteObjectSource();
}

void QConnectedReplicaImplementation::setDisconnected()
{
    Q_ASSERT(connectionToSource);
    connectionToSource.clear();
    setState(QRemoteObjectReplica::State::Suspect);
    for (const int index : childIndices()) {
        auto pointerToQObject = qvariant_cast<QObject *>(getProperty(index));
        auto child = qobject_cast<QRemoteObjectReplica *>(pointerToQObject);
        if (child)
            static_cast<QConnectedReplicaImplementation *>(child->d_impl.data())->setDisconnected();
    }
}

void QConnectedReplicaImplementation::requestRemoteObjectSource()
{
    Q_ASSERT(connectionToSource);
    connectionToSource->d_func()->m_codec->serializeAddObjectPacket(m_objectName, needsDynamicInitialization());
    sendCommand();
}

void QRemoteObjectReplicaImplementation::configurePrivate(QRemoteObjectReplica *rep)
{
    qCDebug(QT_REMOTEOBJECT) << "configurePrivate starting for" << this->m_objectName;
    //We need to connect the Replicant only signals too
    // Uncomment the following two lines if QRemoteObjectDynamicReplica gets a unique staticMetaObject (FIX #3)
    //const QMetaObject *m =  rep->inherits("QRemoteObjectDynamicReplica") ?
    //            &QRemoteObjectDynamicReplica::staticMetaObject : &QRemoteObjectReplica::staticMetaObject;
    const QMetaObject *m = &QRemoteObjectReplica::staticMetaObject;
    for (int i = m->methodOffset(); i < m->methodCount(); ++i)
    {
        const QMetaMethod mm = m->method(i);
        if (mm.methodType() == QMetaMethod::Signal) {
            const bool res = QMetaObject::connect(this, i, rep, i, Qt::DirectConnection, nullptr);
            qCDebug(QT_REMOTEOBJECT) << "  Rep connect"<<i<<res<<mm.name();
            Q_UNUSED(res)
        }
    }
    if (m_methodOffset == 0) //We haven't initialized the offsets yet
    {
        const int index = m_metaObject->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE);
        const QMetaObject *metaObject = m_metaObject;
        if (index != -1) { //We have an object created from repc or at least with QCLASSINFO defined
            while (true) {
                Q_ASSERT(metaObject->superClass()); //This recurses to QObject, which doesn't have QCLASSINFO_REMOTEOBJECT_TYPE
                if (index != metaObject->superClass()->indexOfClassInfo(QCLASSINFO_REMOTEOBJECT_TYPE)) //At the point we don't find the same QCLASSINFO_REMOTEOBJECT_TYPE,
                                //we have the metaobject we should work from
                    break;
                metaObject = metaObject->superClass();
            }
        }

        for (int i = m_signalOffset; i < metaObject->methodCount(); ++i) {
            const QMetaMethod mm = metaObject->method(i);
            if (mm.methodType() == QMetaMethod::Signal) {
                ++m_numSignals;
                const bool res = QMetaObject::connect(this, i, rep, i, Qt::DirectConnection, nullptr);
                qCDebug(QT_REMOTEOBJECT) << "  Connect"<<i<<res<<mm.name();
                Q_UNUSED(res)
            }
        }
        m_methodOffset = m_signalOffset + m_numSignals;
        qCDebug(QT_REMOTEOBJECT) << QStringLiteral("configurePrivate finished, signalOffset = %1, methodOffset = %2, #Signals = %3").arg(m_signalOffset).arg(m_methodOffset).arg(m_numSignals);
    } else { //We have initialized offsets, this is an additional Replica attaching
        for (int i = m_signalOffset; i < m_methodOffset; ++i) {
            const bool res = QMetaObject::connect(this, i, rep, i, Qt::DirectConnection, nullptr);
            qCDebug(QT_REMOTEOBJECT) << "  Connect"<<i<<res<<m_metaObject->method(i).name();
            Q_UNUSED(res)
        }
        if (isInitialized()) {
            qCDebug(QT_REMOTEOBJECT) << QStringLiteral("ReplicaImplementation initialized, emitting signal on replica");
            emit rep->initialized(); //Emit from new replica only
        }
        if (state() != QRemoteObjectReplica::Valid) {
            qCDebug(QT_REMOTEOBJECT) << QStringLiteral("ReplicaImplementation not currently valid, emitting signal on replica");
            emit rep->stateChanged(state(), m_metaObject ? QRemoteObjectReplica::Default : QRemoteObjectReplica::Uninitialized);
        }

        qCDebug(QT_REMOTEOBJECT) << QStringLiteral("configurePrivate finished, added replica to existing ReplicaImplementation");
    }
}

void QConnectedReplicaImplementation::configurePrivate(QRemoteObjectReplica *rep)
{
    if (m_metaObject) {
        // see QRemoteObjectReplicaImplementation::configurePrivate
        const bool firstReplicaInstance = (m_methodOffset == 0);

        QRemoteObjectReplicaImplementation::configurePrivate(rep);

        // ensure that notify signals are emitted for the new replica, when
        // we are initializing an nth replica of the same type
        if (!firstReplicaInstance) {
            const int offset = m_propertyOffset;
            const int nParam = int(m_propertyStorage.size());
            void *args[] = {nullptr, nullptr};
            for (int i = 0; i < nParam; ++i) {
                const int notifyIndex = m_metaObject->property(i+offset).notifySignalIndex();
                if (notifyIndex < 0)
                    continue;
                qCDebug(QT_REMOTEOBJECT) << " Before activate" << notifyIndex << m_metaObject->property(i+offset).name();
                args[1] = m_propertyStorage[i].data();
                // NOTE: this over-emits (assumes all values have changed)
                QMetaObject::activate(rep, rep->metaObject(), notifyIndex - m_signalOffset, args);
            }
        }
    } else
        m_parentsNeedingConnect.append(rep);
}

/*!
    \class QRemoteObjectReplica
    \inmodule QtRemoteObjects
    \brief A class interacting with (but not implementing) a Qt API on the Remote Object network.

    A Remote Object Replica is a QObject proxy for another QObject (called the
    \l {Source} object). Once initialized, a replica can be considered a
    "latent copy" of the \l {Source} object. That is, every change to a
    Q_PROPERTY on the \l {Source}, or signal emitted by the \l {Source} will be
    updated/emitted by all \l {Replica} objects. Latency
    is introduced by process scheduling by any OSes involved and network
    communication latency. As long as the replica has been initialized and the
    communication is not disrupted, receipt and order of changes is guaranteed.

    The \l {isInitialized} and \l {state} properties (and corresponding
    \l {initialized()}/\l {stateChanged()} signals) allow the state of a
    \l {Replica} to be determined.

    While Qt Remote Objects (QtRO) handles the initialization and
    synchronization of \l {Replica} objects, there are numerous steps happening
    behind the scenes which can fail and that aren't encountered in single
    process Qt applications. See \l {Troubleshooting} for advice on how to
    handle such issues when using a remote objects network.
*/

/*!
    \enum QRemoteObjectReplica::State

    This enum type specifies the various state codes associated with QRemoteObjectReplica states:

    \value Uninitialized Initial value of DynamicReplica, where nothing is
    known about the replica before connection to source.

    \value Default Initial value of static replica, where any defaults set in
    the .rep file are available so it can be used if necessary.

    \value Valid Indicates the replica is connected, has good property values
    and can be interacted with.

    \value Suspect Error state that occurs if the connection to the source is
    lost after it is initialized.

    \value SignatureMismatch Error state that occurs if a connection to the
    source is made, but the source and replica are not derived from the same
    .rep (only possible for static Replicas).
*/

/*!
    \fn void QRemoteObjectReplica::stateChanged(State state, State oldState)

    This signal is emitted whenever a replica's state toggles between
    \l QRemoteObjectReplica::State.

    The change in state is represented with \a state and \a oldState.

    \sa state(), initialized()
*/

/*!
    \fn void QRemoteObjectReplica::initialized()

    This signal is emitted once the replica is initialized. An intialized replica
    has all property values set, but has not yet emitted any property change
    notifications.

    \sa isInitialized(), stateChanged()
*/

/*!
    \fn void QRemoteObjectReplica::notified()

    This signal is emitted once the replica is initialized and all property change
    notifications have been emitted.

    It is sometimes useful to respond to property changes as events.
    For example, you might want to display a user notification when a certain
    property change occurs. However, this user notification would then also be
    triggered when a replica first became \c QRemoteObjectReplica::Valid, as
    all property change signals are emitted at that time. This isn't always desirable,
    and \c notified allows the developer to distinguish between these two cases.
*/

/*!
    \internal
    \enum QRemoteObjectReplica::ConstructorType
*/

/*!
    \property QRemoteObjectReplica::state
    \brief Returns the replica state.

    This property holds the replica \l QRemoteObjectReplica::State.
*/

/*!
    \property QRemoteObjectReplica::node
    \brief A pointer to the node this object was acquired from.
*/

/*!
    \internal This (protected) constructor for QRemoteObjectReplica can be used to create
    replica objects from QML.
*/
QRemoteObjectReplica::QRemoteObjectReplica(ConstructorType t)
    : QObject(nullptr)
    , d_impl(t == DefaultConstructor ? new QStubReplicaImplementation : nullptr)
{
    qRegisterMetaType<State>("State");
}

QRemoteObjectReplica::QRemoteObjectReplica(QObjectPrivate &dptr, QObject *parent)
    : QObject(dptr, parent)
    , d_impl(new QStubReplicaImplementation)
{
}

/*!
    \internal
*/
QRemoteObjectReplica::~QRemoteObjectReplica()
{
}

/*!
    \internal
*/
void QRemoteObjectReplica::send(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_ASSERT(index != -1);

    d_impl->_q_send(call, index, args);
}

/*!
    \internal
*/
QRemoteObjectPendingCall QRemoteObjectReplica::sendWithReply(QMetaObject::Call call, int index, const QVariantList &args)
{
    return d_impl->_q_sendWithReply(call, index, args);
}

/*!
    \internal
*/
const QVariant QRemoteObjectReplica::propAsVariant(int i) const
{
    return d_impl->getProperty(i);
}

/*!
    \internal
*/
void QRemoteObjectReplica::initializeNode(QRemoteObjectNode *node, const QString &name)
{
    node->initializeReplica(this, name);
}

/*!
    \internal
*/
void QRemoteObjectReplica::setProperties(QVariantList &&properties)
{
    d_impl->setProperties(std::move(properties));
}

/*!
    \internal
*/
void QRemoteObjectReplica::setChild(int i, const QVariant &value)
{
    d_impl->setProperty(i, value);
}

/*!
    Returns \c true if this replica has been initialized with data from the
    \l {Source} object.  Returns \c false otherwise.

    \sa state()
*/
bool QRemoteObjectReplica::isInitialized() const
{
    return d_impl->isInitialized();
}

/*!
    Returns \c true if this replica has been initialized with data from the
    \l {Source} object.  Returns \c false otherwise.

    \sa isInitialized()
*/
QRemoteObjectReplica::State QRemoteObjectReplica::state() const
{
    return d_impl->state();
}

QRemoteObjectNode *QRemoteObjectReplica::node() const
{
    return d_impl->node();
}

void QRemoteObjectReplica::setNode(QRemoteObjectNode *_node)
{
    const QRemoteObjectNode *curNode = node();
    if (curNode) {
        qCWarning(QT_REMOTEOBJECT) << "Ignoring call to setNode as the node has already been set";
        return;
    }
    d_impl.clear();
    _node->initializeReplica(this);
}

/*!
    \internal
*/
void QRemoteObjectReplica::initialize()
{
}

/*!
    Returns \c true if this replica has been initialized and has a valid
    connection with the \l {QRemoteObjectNode} {node} hosting the \l {Source}.
    Returns \c false otherwise.

    \sa isInitialized()
*/
bool QRemoteObjectReplica::isReplicaValid() const
{
    return state() == Valid;
}

/*!
    Blocking call that waits for the replica to become initialized or until the
    \a timeout (in ms) expires. Returns \c true if the replica is initialized
    when the call completes, \c false otherwise.

    If \a timeout is -1, this function will not time out.

    \sa isInitialized(), initialized()
*/
bool QRemoteObjectReplica::waitForSource(int timeout)
{
    return d_impl->waitForSource(timeout);
}

QInProcessReplicaImplementation::QInProcessReplicaImplementation(const QString &name, const QMetaObject *meta, QRemoteObjectNode * node)
    : QRemoteObjectReplicaImplementation(name, meta, node)
{
}

QInProcessReplicaImplementation::~QInProcessReplicaImplementation()
{
}

const QVariant QInProcessReplicaImplementation::getProperty(int i) const
{
    Q_ASSERT(connectionToSource);
    Q_ASSERT(connectionToSource->m_object);
    const int index = i + QRemoteObjectSource::qobjectPropertyOffset;
    Q_ASSERT(index >= 0 && index < connectionToSource->m_object->metaObject()->propertyCount());
    return connectionToSource->m_object->metaObject()->property(index).read(connectionToSource->m_object);
}

void QInProcessReplicaImplementation::setProperties(QVariantList &&)
{
    //TODO some verification here maybe?
}

void QInProcessReplicaImplementation::setProperty(int i, const QVariant &property)
{
    Q_ASSERT(connectionToSource);
    Q_ASSERT(connectionToSource->m_object);
    const int index = i + QRemoteObjectSource::qobjectPropertyOffset;
    Q_ASSERT(index >= 0 && index < connectionToSource->m_object->metaObject()->propertyCount());
    connectionToSource->m_object->metaObject()->property(index).write(connectionToSource->m_object, property);
}

void QInProcessReplicaImplementation::_q_send(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_ASSERT(call == QMetaObject::InvokeMetaMethod || call == QMetaObject::WriteProperty);

    const SourceApiMap *api = connectionToSource->m_api;
    if (call == QMetaObject::InvokeMetaMethod) {
        const int resolvedIndex = api->sourceMethodIndex(index - m_methodOffset);
        if (resolvedIndex < 0)
            qCWarning(QT_REMOTEOBJECT) << "Skipping invalid invocation.  Index not found:" << index - m_methodOffset;
        else
            connectionToSource->invoke(call, index - m_methodOffset, args);
    } else {
        const int resolvedIndex = connectionToSource->m_api->sourcePropertyIndex(index - m_propertyOffset);
        if (resolvedIndex < 0)
            qCWarning(QT_REMOTEOBJECT) << "Skipping invalid property setter.  Index not found:" << index - m_propertyOffset;
        else
            connectionToSource->invoke(call, index - m_propertyOffset, args);
    }
}

QRemoteObjectPendingCall QInProcessReplicaImplementation::_q_sendWithReply(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_ASSERT(call == QMetaObject::InvokeMetaMethod);

    const int ReplicaIndex = index - m_methodOffset;
    auto metaType = QMetaType::fromName(connectionToSource->m_api->typeName(ReplicaIndex).constData());
    if (!metaType.sizeOf())
        metaType = QMetaType(QMetaType::UnknownType);
    QVariant returnValue(metaType, nullptr);

    const int resolvedIndex = connectionToSource->m_api->sourceMethodIndex(ReplicaIndex);
    if (resolvedIndex < 0) {
        qCWarning(QT_REMOTEOBJECT) << "Skipping invalid invocation.  Index not found:" << ReplicaIndex;
        return QRemoteObjectPendingCall();
    }

    connectionToSource->invoke(call, ReplicaIndex, args, &returnValue);
    return QRemoteObjectPendingCall::fromCompletedCall(returnValue);
}

QStubReplicaImplementation::QStubReplicaImplementation() {}

QStubReplicaImplementation::~QStubReplicaImplementation() {}

const QVariant QStubReplicaImplementation::getProperty(int i) const
{
    Q_ASSERT_X(i >= 0 && i < m_propertyStorage.size(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(i).arg(m_propertyStorage.size())));
    return m_propertyStorage[i];
}

void QStubReplicaImplementation::setProperties(QVariantList &&properties)
{
    Q_ASSERT(m_propertyStorage.isEmpty());
    m_propertyStorage.reserve(properties.size());
    m_propertyStorage = std::move(properties);
}

void QStubReplicaImplementation::setProperty(int i, const QVariant &prop)
{
    m_propertyStorage[i] = prop;
}

void QStubReplicaImplementation::_q_send(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_UNUSED(call)
    Q_UNUSED(index)
    Q_UNUSED(args)
    qWarning("Tried calling a slot or setting a property on a replica that hasn't been initialized with a node");
}

QRemoteObjectPendingCall QStubReplicaImplementation::_q_sendWithReply(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_UNUSED(call)
    Q_UNUSED(index)
    Q_UNUSED(args)
    qWarning("Tried calling a slot or setting a property on a replica that hasn't been initialized with a node");
    return QRemoteObjectPendingCall(); //Invalid
}

QT_END_NAMESPACE
