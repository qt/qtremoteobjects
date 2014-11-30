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

#include "qremoteobjectdynamicreplica.h"
#include "qremoteobjectreplica_p.h"

#include <QMetaProperty>

QT_BEGIN_NAMESPACE

QRemoteObjectDynamicReplica::QRemoteObjectDynamicReplica(QObject *parent)
    : QRemoteObjectReplica(parent)
{
}

QRemoteObjectDynamicReplica::~QRemoteObjectDynamicReplica()
{
}

const QMetaObject* QRemoteObjectDynamicReplica::metaObject() const
{
    Q_D(const QRemoteObjectReplica);

    return d->m_metaObject ? d->m_metaObject : QRemoteObjectReplica::metaObject();
}

void *QRemoteObjectDynamicReplica::qt_metacast(const char *name)
{
    Q_D(QRemoteObjectReplica);

    if (!name)
        return 0;

    if (!strcmp(name, "QRemoteObjectDynamicReplica"))
        return static_cast<void*>(const_cast<QRemoteObjectDynamicReplica*>(this));

    // not entirely sure that one is needed... TODO: check
    if (QString::fromLatin1(name) == d->m_objectName)
        return static_cast<void*>(const_cast<QRemoteObjectDynamicReplica*>(this));

    return QObject::qt_metacast(name);
}

int QRemoteObjectDynamicReplica::qt_metacall(QMetaObject::Call call, int id, void **argv)
{
    Q_D(QRemoteObjectReplica);

    int saved_id = id;
    id = QRemoteObjectReplica::qt_metacall(call, id, argv);
    if (id < 0 || d->m_metaObject == Q_NULLPTR)
        return id;

    if (call == QMetaObject::ReadProperty || call == QMetaObject::WriteProperty) {
        QMetaProperty mp = metaObject()->property(saved_id);
        int &status = *reinterpret_cast<int *>(argv[2]);

        if (call == QMetaObject::WriteProperty) {
            QVariantList args;
            args << QVariant(mp.userType(), argv[0]);
            QRemoteObjectReplica::send(QMetaObject::WriteProperty, saved_id, args);
        } else {
            const QVariant value = propAsVariant(id);
            QMetaType::construct(mp.userType(), argv[0], value.data());
            const bool readStatus = true;
            // Caller supports QVariant returns? Then we can also report errors
            // by storing an invalid variant.
            if (!readStatus && argv[1]) {
                status = 0;
                reinterpret_cast<QVariant*>(argv[1])->clear();
            }
        }

        id = -1;
    } else if (call == QMetaObject::InvokeMetaMethod) {
        if (id < d->m_numSignals) {
            // signal relay from Source world to Replica
            QMetaObject::activate(this, d->m_metaObject, id, argv);

        } else {
            // method relay from Replica to Source
            const int methodIndex = id - d->m_numSignals;
            qCDebug(QT_REMOTEOBJECT) << "method" << d->m_metaObject->method(saved_id).name() << "invoked.  Pending call (" << !d->m_methodReturnTypeIsVoid.at(methodIndex) << ")";

            QVariantList args;
            for (int i = 1; i <= d->m_methodArgumentTypes.at(methodIndex).size(); ++i) {
                args << QVariant(d->m_methodArgumentTypes.at(methodIndex)[i-1], argv[i]);
            }

            const bool returnTypeIsVoid = d->m_methodReturnTypeIsVoid.at(methodIndex);
            if (returnTypeIsVoid)
                QRemoteObjectReplica::send(QMetaObject::InvokeMetaMethod, saved_id, args);
            else {
                QRemoteObjectPendingCall call = QRemoteObjectReplica::sendWithReply(QMetaObject::InvokeMetaMethod, saved_id, args);
                QMetaType::construct(qMetaTypeId<QRemoteObjectPendingCall>(), argv[0], &call);
            }
        }

        id = -1;
    }

    return id;
}

QT_END_NAMESPACE
