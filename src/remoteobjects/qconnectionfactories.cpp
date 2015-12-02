/****************************************************************************
**
** Copyright (C) 2014-2015 Ford Motor Company
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

#include "qconnectionfactories.h"
#include "qconnectionfactories_p.h"

QT_BEGIN_NAMESPACE

using namespace QtRemoteObjects;

inline bool fromDataStream(QDataStream &in, QRemoteObjectPacketTypeEnum &type, QString &name)
{
    quint16 _type;
    in >> _type;
    type = Invalid;
    switch (_type) {
    case InitPacket: type = InitPacket; break;
    case InitDynamicPacket: type = InitDynamicPacket; break;
    case AddObject: type = AddObject; break;
    case RemoveObject: type = RemoveObject; break;
    case InvokePacket: type = InvokePacket; break;
    case InvokeReplyPacket: type = InvokeReplyPacket; break;
    case PropertyChangePacket: type = PropertyChangePacket; break;
    case ObjectList: type = ObjectList; break;
    default:
        qWarning() << "Invalid packet received" << type;
    }
    if (type == Invalid)
        return false;
    if (type == ObjectList)
        return true;
    in >> name;
    return true;
}

ClientIoDevice::ClientIoDevice(QObject *parent)
    : QObject(parent), m_isClosing(false), m_curReadSize(0)
{
    m_dataStream.setVersion(dataStreamVersion);
}

ClientIoDevice::~ClientIoDevice()
{
}

void ClientIoDevice::close()
{
    m_isClosing = true;
    doClose();
}

bool ClientIoDevice::read(QRemoteObjectPacketTypeEnum &type, QString &name)
{
    qCDebug(QT_REMOTEOBJECT) << "ClientIODevice::read()" << m_curReadSize << bytesAvailable();

    if (m_curReadSize == 0) {
        if (bytesAvailable() < static_cast<int>(sizeof(quint32)))
            return false;

        m_dataStream >> m_curReadSize;
    }

    qCDebug(QT_REMOTEOBJECT) << "ClientIODevice::read()-looking for map" << m_curReadSize << bytesAvailable();

    if (bytesAvailable() < m_curReadSize)
        return false;

    m_curReadSize = 0;
    return fromDataStream(m_dataStream, type, name);
}

void ClientIoDevice::write(const QByteArray &data)
{
    connection()->write(data);
}

void ClientIoDevice::write(const QByteArray &data, qint64 size)
{
    connection()->write(data.data(), size);
}

qint64 ClientIoDevice::bytesAvailable()
{
    return connection()->bytesAvailable();
}

QUrl ClientIoDevice::url() const
{
    return m_url;
}

void ClientIoDevice::addSource(const QString &name)
{
    m_remoteObjects.insert(name);
}

void ClientIoDevice::removeSource(const QString &name)
{
    m_remoteObjects.remove(name);
}

QSet<QString> ClientIoDevice::remoteObjects() const
{
    return m_remoteObjects;
}

ServerIoDevice::ServerIoDevice(QObject *parent)
    : QObject(parent), m_isClosing(false), m_curReadSize(0)
{
    m_dataStream.setVersion(dataStreamVersion);
}

ServerIoDevice::~ServerIoDevice()
{
}

bool ServerIoDevice::read(QRemoteObjectPacketTypeEnum &type, QString &name)
{
    qCDebug(QT_REMOTEOBJECT) << "ServerIODevice::read()" << m_curReadSize << bytesAvailable();

    if (m_curReadSize == 0) {
        if (bytesAvailable() < static_cast<int>(sizeof(quint32)))
            return false;

        m_dataStream >> m_curReadSize;
    }

    qCDebug(QT_REMOTEOBJECT) << "ServerIODevice::read()-looking for map" << m_curReadSize << bytesAvailable();

    if (bytesAvailable() < m_curReadSize)
        return false;

    m_curReadSize = 0;
    return fromDataStream(m_dataStream, type, name);
}

void ServerIoDevice::close()
{
    m_isClosing = true;
    doClose();
}

void ServerIoDevice::write(const QByteArray &data)
{
    if (connection()->isOpen() && !m_isClosing)
        connection()->write(data);
}

void ServerIoDevice::write(const QByteArray &data, qint64 size)
{
    if (connection()->isOpen() && !m_isClosing)
        connection()->write(data.data(), size);
}

qint64 ServerIoDevice::bytesAvailable()
{
    return connection()->bytesAvailable();
}

void ServerIoDevice::initializeDataStream()
{
    m_dataStream.setDevice(connection());
    m_dataStream.resetStatus();
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
    ServerIoDevice *iodevice = _nextPendingConnection();
    iodevice->initializeDataStream();
    return iodevice;
}
