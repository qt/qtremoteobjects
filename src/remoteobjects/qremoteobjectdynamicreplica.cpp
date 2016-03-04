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

/*!
    \class QRemoteObjectDynamicReplica
    \inmodule QtRemoteObjects
    \brief A dynamically instantiated \l {Replica}

    There are generated Replicas (ones that have header files produced by the \l {repc} {Replica Compiler}), and dynamic Replicas, which are generated on-the-fly.  This is the class for the dynamic type of Replica.

    When the connection to the \l {Source} object is made, the initialization step passes the current property values (see \l {Replica Initialization}).  In a DynamicReplica, the Property/Signal/Slot details are also sent, allowing the Replica object to be created on-the-fly.  This can be conventient in QML or scripting, but has two primary disadvantages.  First, the object is in effect "empty" until it is successfully initialized by the \l {Source}.  Second, in C++, calls must be made using QMetaObject::invokeMethod(), as the moc generated lookup will not be available.

    This class does not have a public constructor, it can only be instantiated by using the dynamic QRemoteObjectNode::acquire method.
*/

QRemoteObjectDynamicReplica::QRemoteObjectDynamicReplica()
    : QRemoteObjectReplica()
{
}

QRemoteObjectDynamicReplica::QRemoteObjectDynamicReplica(QRemoteObjectNode *node, const QString &name)
    : QRemoteObjectReplica(ConstructWithNode)
{
    initializeNode(node, name);
}

/*!
    Destroys the Dynamic Replica.

    \sa {Replica Ownership}
*/
QRemoteObjectDynamicReplica::~QRemoteObjectDynamicReplica()
{
}

/*!
    \internal
    Returns a pointer to the dynamically generated meta-object of this object, or 0 if the object is not initialized.  This function overrides the QObject::metaObject() virtual function to provide the same functionality for Dynamic Replicas.

    \sa QObject::metaObject(), {Replica Initialization}
*/
const QMetaObject* QRemoteObjectDynamicReplica::metaObject() const
{
    QSharedPointer<QRemoteObjectReplicaPrivate> d = qSharedPointerCast<QRemoteObjectReplicaPrivate>(d_ptr);

    return d->m_metaObject ? d->m_metaObject : QRemoteObjectReplica::metaObject();
}

/*!
    \internal
    This function overrides the QObject::qt_metacast() virtual function to provide the same functionality for Dynamic Replicas.

    \sa QObject::qt_metacast()
*/
void *QRemoteObjectDynamicReplica::qt_metacast(const char *name)
{
    QSharedPointer<QRemoteObjectReplicaPrivate> d = qSharedPointerCast<QRemoteObjectReplicaPrivate>(d_ptr);

    if (!name)
        return 0;

    if (!strcmp(name, "QRemoteObjectDynamicReplica"))
        return static_cast<void*>(const_cast<QRemoteObjectDynamicReplica*>(this));

    // not entirely sure that one is needed... TODO: check
    if (QString::fromLatin1(name) == d->m_objectName)
        return static_cast<void*>(const_cast<QRemoteObjectDynamicReplica*>(this));

    return QObject::qt_metacast(name);
}

/*!
    \internal
    This function overrides the QObject::qt_metacall() virtual function to provide the same functionality for Dynamic Replicas.

    \sa QObject::qt_metacall()
*/
int QRemoteObjectDynamicReplica::qt_metacall(QMetaObject::Call call, int id, void **argv)
{
    static const bool debugArgs = qEnvironmentVariableIsSet("QT_REMOTEOBJECT_DEBUG_ARGUMENTS");

    QSharedPointer<QRemoteObjectReplicaPrivate> d = qSharedPointerCast<QRemoteObjectReplicaPrivate>(d_ptr);

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
            QMetaType::destruct(mp.userType(), argv[0]);
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
            const QMetaMethod mm = d->m_metaObject->method(saved_id);
            const QList<QByteArray> types = mm.parameterTypes();

            const int typeSize = types.size();
            QVariantList args;
            args.reserve(typeSize);
            for (int i = 0; i < typeSize; ++i) {
                args.push_back(QVariant(QVariant::nameToType(types[i].constData()), argv[i + 1]));
            }

            if (debugArgs) {
                qCDebug(QT_REMOTEOBJECT) << "method" << mm.methodSignature() << "invoked - args:" << args;
            } else {
                qCDebug(QT_REMOTEOBJECT) << "method" << mm.methodSignature() << "invoked";
            }

            if (mm.returnType() == QMetaType::Void)
                QRemoteObjectReplica::send(QMetaObject::InvokeMetaMethod, saved_id, args);
            else {
                QRemoteObjectPendingCall call = QRemoteObjectReplica::sendWithReply(QMetaObject::InvokeMetaMethod, saved_id, args);
                if (argv[0])
                    *(static_cast<QRemoteObjectPendingCall*>(argv[0])) = call;
            }
        }

        id = -1;
    }

    return id;
}

QT_END_NAMESPACE
