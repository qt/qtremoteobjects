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

#ifndef QCONNECTIONFACTORIES_P_H
#define QCONNECTIONFACTORIES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qdatastream.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qpointer.h>
#include <QtCore/private/qobject_p.h>

#include <QtRemoteObjects/qtremoteobjectglobal.h>
#include <QtRemoteObjects/qconnectionfactories.h>
#include "qremoteobjectpacket_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtRemoteObjects {

static const int dataStreamVersion = QDataStream::Qt_6_2;
static const QLatin1String protocolVersion("QtRO 2.0");

}

class QtROExternalIoDevice : public QtROIoDeviceBase
{
    Q_OBJECT

public:
    explicit QtROExternalIoDevice(QIODevice *device, QObject *parent=nullptr);
    QIODevice *connection() const override;
    bool isOpen() const override;

protected:
    void doClose() override;
    QString deviceType() const override;
private:
    Q_DECLARE_PRIVATE(QtROExternalIoDevice)
};

class QtROIoDeviceBasePrivate : public QObjectPrivate
{
public:
    QtROIoDeviceBasePrivate();

    // TODO Remove stream()
    QDataStream &stream() { return m_dataStream; }

    bool m_isClosing = false;
    quint32 m_curReadSize = 0;
    QDataStream m_dataStream;
    QSet<QString> m_remoteObjects;
    std::unique_ptr<QRemoteObjectPackets::CodecBase> m_codec { nullptr };
    Q_DECLARE_PUBLIC(QtROIoDeviceBase)
};

class QtROClientIoDevicePrivate : public QtROIoDeviceBasePrivate
{
public:
    QtROClientIoDevicePrivate() : QtROIoDeviceBasePrivate() { }
    QUrl m_url;
    Q_DECLARE_PUBLIC(QtROClientIoDevice)
};

class QtROExternalIoDevicePrivate : public QtROIoDeviceBasePrivate
{
public:
    QtROExternalIoDevicePrivate(QIODevice *device) : QtROIoDeviceBasePrivate(), m_device(device) { }
    QPointer<QIODevice> m_device;
    Q_DECLARE_PUBLIC(QtROExternalIoDevice)
};

QT_END_NAMESPACE

#endif
