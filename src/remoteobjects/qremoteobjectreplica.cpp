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

#include "qremoteobjectreplica.h"
#include "qremoteobjectreplica_p.h"

#include "qremoteobjectdynamicreplica.h"
#include "qremoteobjectpacket_p.h"
#include "qremoteobjectpendingcall_p.h"
#include "qconnectionclientfactory_p.h"
#include "qremoteobjectsource_p.h"
#include "private/qmetaobjectbuilder_p.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QVariant>
#include <QThread>
#include <QTime>
#include <QTimer>

#include <limits>

QT_BEGIN_NAMESPACE

using namespace QRemoteObjectPackets;

QRemoteObjectReplicaPrivate::QRemoteObjectReplicaPrivate(const QString &name, const QMetaObject *meta)
    : QObject(Q_NULLPTR), m_objectName(name), m_metaObject(meta), m_numSignals(0), m_methodOffset(0),
      m_signalOffset(meta ? QRemoteObjectReplica::staticMetaObject.methodCount() : QRemoteObjectDynamicReplica::staticMetaObject.methodCount()),
      m_propertyOffset(meta ? QRemoteObjectReplica::staticMetaObject.propertyCount() : QRemoteObjectDynamicReplica::staticMetaObject.propertyCount())
{
}

QRemoteObjectReplicaPrivate::~QRemoteObjectReplicaPrivate()
{
    if (m_metaObject && qstrcmp(m_metaObject->className(), "QRemoteObjectDynamicReplica") == 0)
        free(const_cast<QMetaObject*>(m_metaObject));
}

QConnectedReplicaPrivate::QConnectedReplicaPrivate(const QString &name, const QMetaObject *meta)
    : QRemoteObjectReplicaPrivate(name, meta), isSet(0), connectionToSource(Q_NULLPTR), m_curSerialId(0)
{
}

QConnectedReplicaPrivate::~QConnectedReplicaPrivate()
{
    if (!connectionToSource.isNull()) {
        qCDebug(QT_REMOTEOBJECT) << "Replica deleted: sending RemoveObject to RemoteObjectSource" << m_objectName;
        QRemoveObjectPacket packet(m_objectName);
        sendCommand(&packet);
    }
}

bool QRemoteObjectReplicaPrivate::needsDynamicInitialization() const
{
    return m_metaObject == Q_NULLPTR;
}

bool QConnectedReplicaPrivate::sendCommand(const QRemoteObjectPacket *packet)
{
    if (connectionToSource.isNull() || !connectionToSource->isOpen()) {
        if (connectionToSource.isNull())
            qCWarning(QT_REMOTEOBJECT) << "connectionToSource is null";
        return false;
    }

    connectionToSource->write(packet->serialize());
    return true;
}
#ifdef Q_COMPILER_UNIFORM_INIT
// QPair (like any class) can be initialized with { }
typedef QPair<int,void*> SignalPair;
inline SignalPair make_pair(int first, void *second)
{ return { qMove(first), qMove(second) }; }
#else
// QPair can't be initialized with { }, need to use a POD
struct SignalPair {
    int first;
    void *second;
};
inline SignalPair make_pair(int first, void *second)
{ SignalPair p = { qMove(first), qMove(second) }; return p; }
#endif

void QConnectedReplicaPrivate::initialize(const QByteArray &packetData)
{
    qCDebug(QT_REMOTEOBJECT) << "initialize()" << m_propertyStorage.size();
    quint32 nParam, len;
    //TODO Make this part of packet deserialize?
    QDataStream in(packetData);
    in.setVersion(QRemoteObjectPackets::dataStreamVersion);
    in >> nParam;
    QVector<SignalPair> signalList;
    signalList.reserve(nParam);
    QVariant value;
    const int offset = m_metaObject->propertyOffset();
    for (quint32 i = 0; i < nParam; ++i) {
        in >> len;
        qint64 pos = in.device()->pos();
        const int index = m_metaObject->indexOfProperty(packetData.constData()+pos) - offset;
        in.skipRawData(len); //Skip property name char *, since we used it in-place ^^
        in >> value;
        qCDebug(QT_REMOTEOBJECT) << "  in loop" << index << m_propertyStorage.size();
        if (index >= 0 && m_propertyStorage[index] != value) {
            m_propertyStorage[index] = value;
            int notifyIndex = m_metaObject->property(index+offset).notifySignalIndex();
            if (notifyIndex >= 0)
                signalList.append(make_pair(notifyIndex, m_propertyStorage[index].data()));
        }
        qCDebug(QT_REMOTEOBJECT) << "SETPROPERTY" << index << m_metaObject->property(index+offset).name() << value.typeName() << value.toString();
    }

    //initialized and validChanged need to be sent manually, since they are not in the derived classes
    //We should emit initialized before emitting any changed signals in case connections are made in a
    //Slot responding to initialized/validChanged.
    if (isSet.fetchAndStoreRelease(2) > 0) {
        //We are already initialized, now we are valid again
        emitValidChanged();
    } else {
        //We need to send the initialized signal, too
        emitInitialized();
        emitValidChanged();
    }

    void *args[] = {Q_NULLPTR, Q_NULLPTR};
    foreach (const SignalPair &row, signalList) {
        qCDebug(QT_REMOTEOBJECT) << " Before activate" << row.first << m_metaObject->property(row.first).name();
        args[1] = row.second;
        QMetaObject::activate(this, metaObject(), row.first, args);
    }

    qCDebug(QT_REMOTEOBJECT) << "isSet = true for" << m_objectName;
}

void QRemoteObjectReplicaPrivate::emitValidChanged()
{
    const static int validChangedIndex = QRemoteObjectReplica::staticMetaObject.indexOfMethod("isReplicaValidChanged()");
    Q_ASSERT(validChangedIndex != -1);
    void *noArgs[] = {0};
    QMetaObject::activate(this, metaObject(), validChangedIndex, noArgs);
}

void QRemoteObjectReplicaPrivate::emitInitialized()
{
    const static int initializedIndex = QRemoteObjectReplica::staticMetaObject.indexOfMethod("initialized()");
    Q_ASSERT(initializedIndex != -1);
    void *noArgs[] = {0};
    QMetaObject::activate(this, metaObject(), initializedIndex, noArgs);
}

void QRemoteObjectReplicaPrivate::initializeMetaObject(const QInitDynamicPacket *packet)
{
    Q_ASSERT(!m_metaObject);

    QMetaObjectBuilder builder;
    builder.setClassName("QRemoteObjectDynamicReplica");
    builder.setSuperClass(&QRemoteObjectReplica::staticMetaObject);
    builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);

    QVector<QPair<QByteArray, QVariant> > propertyValues;
    m_metaObject = packet->createMetaObject(builder, &propertyValues);
    //rely on order of properties;
    QVariantList list;
    typedef QPair<QByteArray, QVariant> PropertyPair;
    foreach (const PropertyPair &pair, propertyValues)
        list << pair.second;
    setProperties(list);
}

void QConnectedReplicaPrivate::initializeMetaObject(const QInitDynamicPacket *packet)
{
    QRemoteObjectReplicaPrivate::initializeMetaObject(packet);
    foreach (QRemoteObjectReplica *obj, m_parentsNeedingConnect)
        configurePrivate(obj);
    m_parentsNeedingConnect.clear();

    //initialized and validChanged need to be sent manually, since they are not in the derived classes
    //We should emit initialized before emitting any changed signals in case connections are made in a
    //Slot responding to initialized/validChanged.
    if (isSet.fetchAndStoreRelease(2) > 0) {
        //We are already initialized, now we are valid again
        emitValidChanged();
    } else {
        //We need to send the initialized signal, too
        emitInitialized();
        emitValidChanged();
    }

    void *args[] = {Q_NULLPTR, Q_NULLPTR};
    for (int index = m_metaObject->propertyOffset(); index < m_metaObject->propertyCount(); ++index) {
        const QMetaProperty mp = m_metaObject->property(index);
        if (mp.hasNotifySignal()) {
            qCDebug(QT_REMOTEOBJECT) << " Before activate" << index << m_metaObject->property(index).name();
            args[1] = this->m_propertyStorage[index-m_propertyOffset].data();
            QMetaObject::activate(this, metaObject(), mp.notifySignalIndex(), args);
        }
    }

    qCDebug(QT_REMOTEOBJECT) << "isSet = true for" << m_objectName;
}

bool QConnectedReplicaPrivate::isInitialized() const
{
    return isSet.load() > 0;
}

bool QConnectedReplicaPrivate::isReplicaValid() const
{
    qCDebug(QT_REMOTEOBJECT) << "isReplicaValid()" << isSet.load();

    return isSet.load() == 2;
}

bool QConnectedReplicaPrivate::waitForSource(int timeout)
{
    if (isSet.load() != 2) {
        QTime t;
        t.start();

        while (isSet.load() != 2) {
            if (t.elapsed() > timeout) {
                qCWarning(QT_REMOTEOBJECT) << "Timeout waiting for client to get set" << m_objectName;
                return false;
            }

            qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 10);
        }
    }

    return true;
}

void QConnectedReplicaPrivate::_q_send(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_ASSERT(call == QMetaObject::InvokeMetaMethod || call == QMetaObject::WriteProperty);

    if (call == QMetaObject::InvokeMetaMethod) {
        qCDebug(QT_REMOTEOBJECT) << "Send" << call << this->m_metaObject->method(index).name() << index << args << connectionToSource;
        if (index < m_methodOffset) //index - m_methodOffset < 0 is invalid, and can't be resolved on the Source side
            qCWarning(QT_REMOTEOBJECT) << "Skipping invalid method invocation.  Index not found:" << index << "( offset =" << m_methodOffset << ") object:" << m_objectName << this->m_metaObject->method(index).name();
        else {
            QInvokePacket package = QInvokePacket(m_objectName, call, index - m_methodOffset, args);
            sendCommand(&package);
        }
    } else {
        qCDebug(QT_REMOTEOBJECT) << "Send" << call << this->m_metaObject->property(index).name() << index << args << connectionToSource;
        if (index < m_propertyOffset) //index - m_propertyOffset < 0 is invalid, and can't be resolved on the Source side
            qCWarning(QT_REMOTEOBJECT) << "Skipping invalid property invocation.  Index not found:" << index << "( offset =" << m_propertyOffset << ") object:" << m_objectName << this->m_metaObject->property(index).name();
        else {
            QInvokePacket package = QInvokePacket(m_objectName, call, index - m_propertyOffset, args);
            sendCommand(&package);
        }
    }
}

QRemoteObjectPendingCall QConnectedReplicaPrivate::_q_sendWithReply(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_ASSERT(call == QMetaObject::InvokeMetaMethod);

    qCDebug(QT_REMOTEOBJECT) << "Send" << call << this->m_metaObject->method(index).name() << index << args << connectionToSource;
    QInvokePacket package = QInvokePacket(m_objectName, call, index - m_methodOffset, args);
    return sendCommandWithReply(&package);
}

QRemoteObjectPendingCall QConnectedReplicaPrivate::sendCommandWithReply(QRemoteObjectPackets::QInvokePacket* packet)
{
    Q_ASSERT(packet);
    packet->serialId = (m_curSerialId == std::numeric_limits<int>::max() ? 0 : m_curSerialId++);

    bool success = sendCommand(packet);
    if (!success) {
        return QRemoteObjectPendingCall(); // invalid
    }

    qCDebug(QT_REMOTEOBJECT) << "Sent InvokePacket with serial id:" << packet->serialId;
    QRemoteObjectPendingCall pendingCall(new QRemoteObjectPendingCallData(packet->serialId, this));
    Q_ASSERT(!m_pendingCalls.contains(packet->serialId));
    m_pendingCalls[packet->serialId] = pendingCall;
    return pendingCall;
}

void QConnectedReplicaPrivate::notifyAboutReply(const QRemoteObjectPackets::QInvokeReplyPacket* replyPacket)
{
    QRemoteObjectPendingCall call = m_pendingCalls.take(replyPacket->ackedSerialId);

    QMutexLocker mutex(&call.d->mutex);

    // clear error flag
    call.d->error = QRemoteObjectPendingCall::NoError;
    call.d->returnValue = replyPacket->value;

    // notify watchers if needed
    if (call.d->watcherHelper)
        call.d->watcherHelper->emitSignals();
}

bool QConnectedReplicaPrivate::waitForFinished(const QRemoteObjectPendingCall& call, int timeout)
{
    if (!call.d->watcherHelper)
        call.d->watcherHelper.reset(new QRemoteObjectPendingCallWatcherHelper);

    call.d->mutex.unlock();

    QEventLoop loop;
    loop.connect(call.d->watcherHelper.data(), SIGNAL(finished()), SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));

    // enter the event loop and wait for a reply
    loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);

    call.d->mutex.lock();

    return call.d->error != QRemoteObjectPendingCall::InvalidMessage;
}

const QVariant QConnectedReplicaPrivate::getProperty(int i) const
{
    Q_ASSERT_X(i >= 0 && i < m_propertyStorage.size(), __FUNCTION__, qPrintable(QString(QLatin1String("0 <= %1 < %2")).arg(i).arg(m_propertyStorage.size())));
    return m_propertyStorage[i];
}

void QConnectedReplicaPrivate::setProperties(const QVariantList &properties)
{
    Q_ASSERT(m_propertyStorage.isEmpty());
    m_propertyStorage.reserve(properties.length());
    m_propertyStorage = properties;
}

void QConnectedReplicaPrivate::setProperty(int i, const QVariant &prop)
{
    m_propertyStorage[i] = prop;
}

void QConnectedReplicaPrivate::setConnection(ClientIoDevice *conn)
{
    if (connectionToSource.isNull()) {
        connectionToSource = conn;
        qCDebug(QT_REMOTEOBJECT) << "setConnection started" << conn << m_objectName;
    }
    requestRemoteObjectSource();
}

void QConnectedReplicaPrivate::setDisconnected()
{
    connectionToSource.clear();
    if (isSet.fetchAndStoreRelease(1) == 2)
        emitValidChanged();
}

void QConnectedReplicaPrivate::requestRemoteObjectSource()
{
    QAddObjectPacket packet(m_objectName, needsDynamicInitialization());
    sendCommand(&packet);
}

void QRemoteObjectReplicaPrivate::configurePrivate(QRemoteObjectReplica *rep)
{
    qCDebug(QT_REMOTEOBJECT) << "configurePrivate starting for" << this->m_objectName;
    //We need to connect the Replicant only signals too
    const QMetaObject *m =  qobject_cast<QRemoteObjectDynamicReplica *>(rep) ?
                &QRemoteObjectReplica::staticMetaObject : &QRemoteObjectDynamicReplica::staticMetaObject;
    for (int i = m->methodOffset(); i < m->methodCount(); ++i)
    {
        const QMetaMethod mm = m->method(i);
        if (mm.methodType() == QMetaMethod::Signal) {
            const bool res = QMetaObject::connect(this, i, rep, i, Qt::DirectConnection, 0);
            qCDebug(QT_REMOTEOBJECT) << "  Rep connect"<<i<<res<<mm.name();
            Q_UNUSED(res);
        }
    }
    if (m_methodOffset == 0) //We haven't initialized the offsets yet
    {
        for (int i = m_signalOffset; i < m_metaObject->methodCount(); ++i) {
            const QMetaMethod mm = m_metaObject->method(i);
            if (mm.methodType() == QMetaMethod::Signal) {
                ++m_numSignals;
                const bool res = QMetaObject::connect(this, i, rep, i, Qt::DirectConnection, 0);
                qCDebug(QT_REMOTEOBJECT) << "  Connect"<<i<<res<<mm.name();
                Q_UNUSED(res);
            }
        }
        m_methodOffset = m_signalOffset + m_numSignals;
        qCDebug(QT_REMOTEOBJECT) << QStringLiteral("configurePrivate finished, signalOffset = %1, methodOffset = %2, #Signals = %3").arg(m_signalOffset).arg(m_methodOffset).arg(m_numSignals);
    } else { //We have initialized offsets, this is an additional Replica attaching
        for (int i = m_signalOffset; i < m_methodOffset; ++i) {
            const bool res = QMetaObject::connect(this, i, rep, i, Qt::DirectConnection, 0);
            qCDebug(QT_REMOTEOBJECT) << "  Connect"<<i<<res<<m_metaObject->method(i).name();
            Q_UNUSED(res);
        }
        if (isInitialized()) {
            qCDebug(QT_REMOTEOBJECT) << QStringLiteral("ReplicaPrivate initialized, emitting signal on Replica");
            emit rep->initialized(); //Emit from new replica only
        }
        if (!isReplicaValid()) {
            qCDebug(QT_REMOTEOBJECT) << QStringLiteral("ReplicaPrivate not currently valid, emitting signal on Replica");
            emit rep->isReplicaValidChanged(); //Emit from new replica only
        }
        qCDebug(QT_REMOTEOBJECT) << QStringLiteral("configurePrivate finished, added Replica to existing ReplicaPrivate");
    }
}

void QConnectedReplicaPrivate::configurePrivate(QRemoteObjectReplica *rep)
{
    if (m_metaObject)
        QRemoteObjectReplicaPrivate::configurePrivate(rep);
    else
        m_parentsNeedingConnect.append(rep);
}

/*!
    \class QRemoteObjectReplica
    \inmodule QtRemoteObjects
    \brief A class interacting with (but not implementing) a Qt API on the Remote Object network

    A Remote Object Replica is a QObject proxy for another QObject (called the
    \l {Source} object). Once initialized, a Replica can be considered a
    "latent copy" of the \l {Source} object. That is, every change to a
    Q_PROPERTY on the \l {Source}, or Signal emitted by the \l {Source} will be
    updated/emitted by all \l {Replica} objects. Latency
    is introduced by process scheduling by any OSes involved and network
    communication latency. As long as the Replica has been initialized and the
    communication is not disrupted, receipt and order of changes is guaranteed.

    The \l {isInitialized} and \l {isReplicaValid} properties (and corresponding \l {initialized()}/\l {isReplicaValidChanged()} Signals) allow the state of a \l {Replica} to be determined.

    While Qt Remote Objects (QtRO) handles the initialization and synchronization of \l {Replica} objects, there are numerous steps happening behind the scenes which can fail and that aren't encountered in single process Qt applications.  See \l {Troubleshooting} for advice on how to handle such issues when using a Remote Objects network.
*/

/*!
    \fn void QRemoteObjectReplica::isReplicaValidChanged()

    This signal is emitted whenever a Replica's state toggles between valid and invalid.

    A Replica is valid when there is a connection between its Node and the Source objects Host Node, and the Replica has been initialized.

    \sa isReplicaValid(), initialized()
*/

/*!
    \fn void QRemoteObjectReplica::initialized()

    This signal is emitted once the Replica is initialized.

    \sa isInitialized(), isReplicaValidChanged()
*/

/*!
    \property QRemoteObjectReplica::isReplicaValid
    \brief Whether the Replica is valid or not.

    This property will be false until the Replica is initialized, at which point it is set to true.  If the connection to the Host Node (of the \l {Source}) is lost, it will become false until the connection is restored.
*/

/*!
    \internal
*/
QRemoteObjectReplica::QRemoteObjectReplica(QObject *parent) : QObject(parent)
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
    Q_D(QRemoteObjectReplica);

    Q_ASSERT(index != -1);

    d->_q_send(call, index, args);
}

/*!
    \internal
*/
QRemoteObjectPendingCall QRemoteObjectReplica::sendWithReply(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_D(QRemoteObjectReplica);

    return d->_q_sendWithReply(call, index, args);
}

/*!
    \internal
*/
const QVariant QRemoteObjectReplica::propAsVariant(int i) const
{
    Q_D(const QRemoteObjectReplica);
    return d->getProperty(i);
}

/*!
    \internal
*/
void QRemoteObjectReplica::setProperties(const QVariantList &properties)
{
    Q_D(QRemoteObjectReplica);
    d->setProperties(properties);
}

/*!
    \internal
*/
void QRemoteObjectReplica::setProperty(int i, const QVariant &prop)
{
    Q_D(QRemoteObjectReplica);
    d->setProperty(i, prop);
}

/*!
    Returns \c true if this Replica has been initialized with data from the \l {Source} object.  Returns \c false otherwise.

    \sa isReplicaValid()
*/
bool QRemoteObjectReplica::isInitialized() const
{
    Q_D(const QRemoteObjectReplica);

    return d->isInitialized();
}

/*!
    \internal
*/
void QRemoteObjectReplica::initialize()
{
}

/*!
    Returns \c true if this Replica has been initialized and has a valid connection with the \l {QRemoteObjectNode} {Node} hosting the \l {Source}.  Returns \c false otherwise.

    \sa isInitialized()
*/
bool QRemoteObjectReplica::isReplicaValid() const
{
    Q_D(const QRemoteObjectReplica);

    return d->isReplicaValid();
}

/*!
    Blocking call that waits for the Replica to become initialized or until the \a timeout (in ms) expires.  Returns \c true if the Replica is initialized when the call completes, \c false otherwise.

    \sa isInitialized(), initialized()
*/
bool QRemoteObjectReplica::waitForSource(int timeout)
{
    Q_D(QRemoteObjectReplica);

    return d->waitForSource(timeout);
}

QInProcessReplicaPrivate::QInProcessReplicaPrivate(const QString &name, const QMetaObject *meta) : QRemoteObjectReplicaPrivate(name, meta)
{
}

QInProcessReplicaPrivate::~QInProcessReplicaPrivate()
{
}

const QVariant QInProcessReplicaPrivate::getProperty(int i) const
{
    Q_ASSERT(connectionToSource);
    Q_ASSERT(connectionToSource->m_object);
    const int index = i + QRemoteObjectSource::qobjectPropertyOffset;
    Q_ASSERT(index >= 0 && index < connectionToSource->m_object->metaObject()->propertyCount());
    return connectionToSource->m_object->metaObject()->property(index).read(connectionToSource->m_object);
}

void QInProcessReplicaPrivate::setProperties(const QVariantList &)
{
    //TODO some verification here maybe?
}

void QInProcessReplicaPrivate::setProperty(int i, const QVariant &property)
{
    Q_ASSERT(connectionToSource);
    Q_ASSERT(connectionToSource->m_object);
    const int index = i + QRemoteObjectSource::qobjectPropertyOffset;
    Q_ASSERT(index >= 0 && index < connectionToSource->m_object->metaObject()->propertyCount());
    connectionToSource->m_object->metaObject()->property(index).write(connectionToSource->m_object, property);
}

void QInProcessReplicaPrivate::_q_send(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_ASSERT(call == QMetaObject::InvokeMetaMethod || call == QMetaObject::WriteProperty);

    const SourceApiMap *api = connectionToSource->m_api;
    if (call == QMetaObject::InvokeMetaMethod) {
        const int resolvedIndex = api->sourceMethodIndex(index - m_methodOffset);
        if (resolvedIndex < 0)
            qCWarning(QT_REMOTEOBJECT) << "Skipping invalid invocation.  Index not found:" << index - m_methodOffset;
        else
            connectionToSource->invoke(call, api->isAdapterMethod(index - m_methodOffset), resolvedIndex, args);
    } else {
        const int resolvedIndex = connectionToSource->m_api->sourcePropertyIndex(index - m_propertyOffset);
        if (resolvedIndex < 0)
            qCWarning(QT_REMOTEOBJECT) << "Skipping invalid property setter.  Index not found:" << index - m_propertyOffset;
        else
            connectionToSource->invoke(call, api->isAdapterProperty(index - m_propertyOffset), resolvedIndex, args);
    }
}

QRemoteObjectPendingCall QInProcessReplicaPrivate::_q_sendWithReply(QMetaObject::Call call, int index, const QVariantList &args)
{
    Q_ASSERT(call == QMetaObject::InvokeMetaMethod);

    const int ReplicaIndex = index - m_methodOffset;
    int typeId = QVariant::nameToType(connectionToSource->m_api->typeName(ReplicaIndex).constData());
    if (!QMetaType(typeId).sizeOf())
        typeId = QVariant::Invalid;
    QVariant returnValue(typeId, Q_NULLPTR);

    const int resolvedIndex = connectionToSource->m_api->sourceMethodIndex(ReplicaIndex);
    if (resolvedIndex < 0) {
        qCWarning(QT_REMOTEOBJECT) << "Skipping invalid invocation.  Index not found:" << ReplicaIndex;
        return QRemoteObjectPendingCall();
    }

    connectionToSource->invoke(call, connectionToSource->m_api->isAdapterMethod(ReplicaIndex), resolvedIndex, args, &returnValue);
    return QRemoteObjectPendingCall::fromCompletedCall(returnValue);
}

QT_END_NAMESPACE
