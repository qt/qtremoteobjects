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

#include "qconnectionabstractserver_p.h"
#include "qtremoteobjectglobal.h"

QT_BEGIN_NAMESPACE

ServerIoDevice::ServerIoDevice(QObject *parent)
    : QObject(parent), m_isClosing(false), m_curReadSize(0), m_packet(Q_NULLPTR)
{
}

ServerIoDevice::~ServerIoDevice()
{
    delete m_packet;
}

bool ServerIoDevice::read()
{
    qCDebug(QT_REMOTEOBJECT) << "ServerIODevice::read()" << m_curReadSize << bytesAvailable();

    QDataStream in(connection());
    if (m_curReadSize == 0) {
        if (bytesAvailable() < static_cast<int>(sizeof(quint32)))
            return false;

        in >> m_curReadSize;
    }

    qCDebug(QT_REMOTEOBJECT) << "ServerIODevice::read()-looking for map" << m_curReadSize << bytesAvailable();

    if (bytesAvailable() < m_curReadSize)
        return false;

    m_curReadSize = 0;
    delete m_packet;
    m_packet = QRemoteObjectPackets::QRemoteObjectPacket::fromDataStream(in);
    return  m_packet && m_packet->id != QRemoteObjectPackets::QRemoteObjectPacket::Invalid;
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

qint64 ServerIoDevice::bytesAvailable()
{
    return connection()->bytesAvailable();
}

QRemoteObjectPackets::QRemoteObjectPacket *ServerIoDevice::packet() const
{
    return m_packet;
}


QConnectionAbstractServer::QConnectionAbstractServer(QObject *parent)
    : QObject(parent)
{
}

QConnectionAbstractServer::~QConnectionAbstractServer()
{
}

QT_END_NAMESPACE
