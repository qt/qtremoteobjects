/****************************************************************************
**
** Copyright (C) 2017-2015 Ford Motor Company
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

#include <QtCore/private/qobject_p.h>

#include "qconnectionfactories_p.h"
#include "qconnectionfactories_p.h"

// BEGIN: Backends
#if defined(Q_OS_QNX)
#include "qconnection_qnx_backend_p.h"
#endif
#include "qconnection_local_backend_p.h"
#include "qconnection_tcpip_backend_p.h"
// END: Backends

QT_BEGIN_NAMESPACE

using namespace QtRemoteObjects;

class QtROFactoryLoader
{
public:
    QtROClientFactory clientFactory;
    QtROServerFactory serverFactory;
};

Q_GLOBAL_STATIC(QtROFactoryLoader, loader)

class IoDeviceBasePrivate : public QObjectPrivate
{
public:
    IoDeviceBasePrivate() : QObjectPrivate() { m_dataStream.setVersion(dataStreamVersion); }
    ~IoDeviceBasePrivate() = default;
    bool m_isClosing = false;
    quint32 m_curReadSize = 0;
    QDataStream m_dataStream;
    QSet<QString> m_remoteObjects;
    Q_DECLARE_PUBLIC(IoDeviceBase)
};

class ClientIoDevicePrivate : public IoDeviceBasePrivate
{
public:
    ClientIoDevicePrivate() : IoDeviceBasePrivate() { }
    QUrl m_url;
    Q_DECLARE_PUBLIC(ClientIoDevice)
};

class ExternalIoDevicePrivate : public IoDeviceBasePrivate
{
public:
    ExternalIoDevicePrivate(QIODevice *device) : IoDeviceBasePrivate(), m_device(device) { }
    QPointer<QIODevice> m_device;
    Q_DECLARE_PUBLIC(ExternalIoDevice)
};

inline bool fromDataStream(QDataStream &in, QRemoteObjectPacketTypeEnum &type, QString &name)
{
    quint16 _type;
    in >> _type;
    type = Invalid;
    switch (_type) {
    case Handshake: type = Handshake; break;
    case InitPacket: type = InitPacket; break;
    case InitDynamicPacket: type = InitDynamicPacket; break;
    case AddObject: type = AddObject; break;
    case RemoveObject: type = RemoveObject; break;
    case InvokePacket: type = InvokePacket; break;
    case InvokeReplyPacket: type = InvokeReplyPacket; break;
    case PropertyChangePacket: type = PropertyChangePacket; break;
    case ObjectList: type = ObjectList; break;
    case Ping: type = Ping; break;
    case Pong: type = Pong; break;
    default:
        qCWarning(QT_REMOTEOBJECT_IO) << "Invalid packet received" << _type;
    }
    if (type == Invalid)
        return false;
    if (type == ObjectList)
        return true;
    in >> name;
    qCDebug(QT_REMOTEOBJECT_IO) << "Packet received of type" << type << "for object" << name;
    return true;
}

/*!
    All communication between nodes happens through some form of QIODevice with
    an associated QDataStream to handle marshalling of Qt types. IoDeviceBase
    is an abstract base class that provides a consistent interface to QtRO, yet
    can be extended to support different types of QIODevice.
 */
IoDeviceBase::IoDeviceBase(QObject *parent) : QObject(*new IoDeviceBasePrivate, parent) { }

IoDeviceBase::IoDeviceBase(IoDeviceBasePrivate &dptr, QObject *parent) : QObject(dptr, parent) { }

IoDeviceBase::~IoDeviceBase()
{
}

bool IoDeviceBase::read(QRemoteObjectPacketTypeEnum &type, QString &name)
{
    Q_D(IoDeviceBase);
    qCDebug(QT_REMOTEOBJECT_IO) << deviceType() << "read()" << d->m_curReadSize << bytesAvailable();

    if (d->m_curReadSize == 0) {
        if (bytesAvailable() < static_cast<int>(sizeof(quint32)))
            return false;

        d->m_dataStream >> d->m_curReadSize;
    }

    qCDebug(QT_REMOTEOBJECT_IO) << deviceType() << "read()-looking for map" << d->m_curReadSize
                                << bytesAvailable();

    if (bytesAvailable() < d->m_curReadSize)
        return false;

    d->m_curReadSize = 0;
    return fromDataStream(d->m_dataStream, type, name);
}

void IoDeviceBase::write(const QByteArray &data)
{
    Q_D(IoDeviceBase);
    if (connection()->isOpen() && !d->m_isClosing)
        connection()->write(data);
}

void IoDeviceBase::write(const QByteArray &data, qint64 size)
{
    Q_D(IoDeviceBase);
    if (connection()->isOpen() && !d->m_isClosing)
        connection()->write(data.data(), size);
}

bool IoDeviceBase::isOpen() const
{
    return !isClosing();
}

void IoDeviceBase::close()
{
    Q_D(IoDeviceBase);
    d->m_isClosing = true;
    doClose();
}

qint64 IoDeviceBase::bytesAvailable() const
{
    return connection()->bytesAvailable();
}

void IoDeviceBase::initializeDataStream()
{
    Q_D(IoDeviceBase);
    d->m_dataStream.setDevice(connection());
    d->m_dataStream.resetStatus();
}

QDataStream &IoDeviceBase::stream()
{
    Q_D(IoDeviceBase);
    return d->m_dataStream;
}

bool IoDeviceBase::isClosing() const
{
    Q_D(const IoDeviceBase);
    return d->m_isClosing;
}

void IoDeviceBase::addSource(const QString &name)
{
    Q_D(IoDeviceBase);
    d->m_remoteObjects.insert(name);
}

void IoDeviceBase::removeSource(const QString &name)
{
    Q_D(IoDeviceBase);
    d->m_remoteObjects.remove(name);
}

QSet<QString> IoDeviceBase::remoteObjects() const
{
    Q_D(const IoDeviceBase);
    return d->m_remoteObjects;
}

ClientIoDevice::ClientIoDevice(QObject *parent) : IoDeviceBase(*new ClientIoDevicePrivate, parent)
{
}

ClientIoDevice::~ClientIoDevice()
{
    if (!isClosing())
        close();
}

void ClientIoDevice::disconnectFromServer()
{
    doDisconnectFromServer();
    emit shouldReconnect(this);
}

QUrl ClientIoDevice::url() const
{
    Q_D(const ClientIoDevice);
    return d->m_url;
}

QString ClientIoDevice::deviceType() const
{
    return QStringLiteral("ClientIoDevice");
}

void ClientIoDevice::setUrl(const QUrl &url)
{
    Q_D(ClientIoDevice);
    d->m_url = url;
}

/*!
    The Qt servers create QIODevice derived classes from handleConnection. The
    problem is that they behave differently, so this class adds some
    consistency.
 */
ServerIoDevice::ServerIoDevice(QObject *parent) : IoDeviceBase(parent)
{
}

QString ServerIoDevice::deviceType() const
{
    return QStringLiteral("ServerIoDevice");
}

QConnectionAbstractServer::QConnectionAbstractServer(QObject *parent)
    : QObject(parent)
{
}

QConnectionAbstractServer::~QConnectionAbstractServer()
{
}

ServerIoDevice *QConnectionAbstractServer::nextPendingConnection()
{
    ServerIoDevice *iodevice = configureNewConnection();
    iodevice->initializeDataStream();
    return iodevice;
}

ExternalIoDevice::ExternalIoDevice(QIODevice *device, QObject *parent)
    : IoDeviceBase(*new ExternalIoDevicePrivate(device), parent)
{
    Q_D(ExternalIoDevice);
    initializeDataStream();
    connect(device, &QIODevice::aboutToClose, this, [d]() { d->m_isClosing = true; });
    connect(device, &QIODevice::readyRead, this, &ExternalIoDevice::readyRead);
    auto meta = device->metaObject();
    if (-1 != meta->indexOfSignal(SIGNAL(disconnected())))
        connect(device, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
}

QIODevice *ExternalIoDevice::connection() const
{
    Q_D(const ExternalIoDevice);
    return d->m_device;
}

bool ExternalIoDevice::isOpen() const
{
    Q_D(const ExternalIoDevice);
    if (!d->m_device)
        return false;
    return d->m_device->isOpen() && IoDeviceBase::isOpen();
}

void ExternalIoDevice::doClose()
{
    Q_D(ExternalIoDevice);
    if (isOpen())
        d->m_device->close();
}

QString ExternalIoDevice::deviceType() const
{
    return QStringLiteral("ExternalIoDevice");
}

/*!
    \class QtROServerFactory
    \inmodule QtRemoteObjects
    \brief A class that holds information about server backends available on the Qt Remote Objects network.
*/
QtROServerFactory::QtROServerFactory()
{
#if defined(Q_OS_QNX)
    registerType<QnxServerImpl>(QStringLiteral("qnx"));
#endif
    registerType<LocalServerImpl>(QStringLiteral("local"));
    registerType<TcpServerImpl>(QStringLiteral("tcp"));
}

QtROServerFactory *QtROServerFactory::instance()
{
    return &loader->serverFactory;
}

/*!
    \class QtROClientFactory
    \inmodule QtRemoteObjects
    \brief A class that holds information about client backends available on the Qt Remote Objects network.
*/
QtROClientFactory::QtROClientFactory()
{
#if defined(Q_OS_QNX)
    registerType<QnxClientIo>(QStringLiteral("qnx"));
#endif
    registerType<LocalClientIo>(QStringLiteral("local"));
    registerType<TcpClientIo>(QStringLiteral("tcp"));
}

QtROClientFactory *QtROClientFactory::instance()
{
    return &loader->clientFactory;
}

/*!
    \fn void qRegisterRemoteObjectsClient(const QString &id)
    \relates QtROClientFactory

    Registers the Remote Objects client \a id for the type \c{T}.

    If you need a custom transport protocol for Qt Remote Objects, you need to
    register the client & server implementation here.

    \note This function requires that \c{T} is a fully defined type at the point
    where the function is called.

    This example registers the class \c{CustomClientIo} as \c{"myprotocol"}:

    \code
        qRegisterRemoteObjectsClient<CustomClientIo>(QStringLiteral("myprotocol"));
    \endcode

    With this in place, you can now instantiate nodes using this new custom protocol:

    \code
        QRemoteObjectNode client(QUrl(QStringLiteral("myprotocol:registry")));
    \endcode

    \sa {qRegisterRemoteObjectsServer}
*/

/*!
    \fn void qRegisterRemoteObjectsServer(const QString &id)
    \relates QtROServerFactory

    Registers the Remote Objects server \a id for the type \c{T}.

    If you need a custom transport protocol for Qt Remote Objects, you need to
    register the client & server implementation here.

    \note This function requires that \c{T} is a fully defined type at the point
    where the function is called.

    This example registers the class \c{CustomServerImpl} as \c{"myprotocol"}:

    \code
        qRegisterRemoteObjectsServer<CustomServerImpl>(QStringLiteral("myprotocol"));
    \endcode

    With this in place, you can now instantiate nodes using this new custom protocol:

    \code
        QRemoteObjectNode client(QUrl(QStringLiteral("myprotocol:registry")));
    \endcode

    \sa {qRegisterRemoteObjectsServer}
*/

QT_END_NAMESPACE
