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

#include "qremoteobjectregistry.h"
#include "qremoteobjectreplica_p.h"

#include <QDataStream>

QT_BEGIN_NAMESPACE

QRemoteObjectRegistry::QRemoteObjectRegistry(QObject *parent) : QRemoteObjectReplica(parent)
{
}

QRemoteObjectRegistry::~QRemoteObjectRegistry()
{}

void QRemoteObjectRegistry::initialize()
{
    qRegisterMetaType<QRemoteObjectSourceLocation>();
    qRegisterMetaTypeStreamOperators<QRemoteObjectSourceLocation>();
    qRegisterMetaType<QRemoteObjectSourceLocations>();
    qRegisterMetaTypeStreamOperators<QRemoteObjectSourceLocations>();
    QVariantList properties;
    properties.reserve(3);
    properties << QVariant::fromValue(QRemoteObjectSourceLocations());
    properties << QVariant::fromValue(QRemoteObjectSourceLocation());
    properties << QVariant::fromValue(QRemoteObjectSourceLocation());
    setProperties(properties);
}

QRemoteObjectSourceLocations QRemoteObjectRegistry::sourceLocations() const
{
    return propAsVariant(0).value<QRemoteObjectSourceLocations>();
}

void QRemoteObjectRegistry::addSource(const QRemoteObjectSourceLocation &entry)
{
    if (!isInitialized()) {
        bool res = waitForSource();
        if (!res)
            return; //FIX What to do here?
    }
    qCDebug(QT_REMOTEOBJECT) << "An entry was added to the registry - Sending to Source" << entry.first << entry.second;
    // This does not set any data to avoid a coherency problem between client and server
    static int index = QRemoteObjectRegistry::staticMetaObject.indexOfMethod("addSource(QRemoteObjectSourceLocation)");
    QVariantList args;
    args << QVariant::fromValue(entry);
    send(QMetaObject::InvokeMetaMethod, index, args);
}

void QRemoteObjectRegistry::removeSource(const QRemoteObjectSourceLocation &entry)
{
    qCDebug(QT_REMOTEOBJECT) << "An entry was removed from the registry - Sending to Source" << entry.first << entry.second;
    // This does not set any data to avoid a coherency problem between client and server
    static int index = QRemoteObjectRegistry::staticMetaObject.indexOfMethod("removeSource(QRemoteObjectSourceLocation)");
    QVariantList args;
    args << QVariant::fromValue(entry);
    send(QMetaObject::InvokeMetaMethod, index, args);
}

QT_END_NAMESPACE
