/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QREMOTEOBJECTREGISTRY_P_H
#define QREMOTEOBJECTREGISTRY_P_H

#include <QtRemoteObjects/qremoteobjectreplica.h>

QT_BEGIN_NAMESPACE

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectRegistry : public QRemoteObjectReplica
{
    Q_OBJECT
    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, "Registry")

    Q_PROPERTY(QRemoteObjectSourceLocations sourceLocations READ sourceLocations)

    friend class QRemoteObjectNode;

public:
    ~QRemoteObjectRegistry();

    QRemoteObjectSourceLocations sourceLocations() const;

Q_SIGNALS:
    void remoteObjectAdded(const QRemoteObjectSourceLocation &entry);
    void remoteObjectRemoved(const QRemoteObjectSourceLocation &entry);

protected Q_SLOTS:
    void addSource(const QRemoteObjectSourceLocation &entry);
    void removeSource(const QRemoteObjectSourceLocation &entry);
    void pushToRegistryIfNeeded();

private:
    void initialize() Q_DECL_OVERRIDE;
    explicit QRemoteObjectRegistry();
    explicit QRemoteObjectRegistry(QRemoteObjectNode *node, const QString &name);
    QRemoteObjectSourceLocations hostedSources;
    friend class QRemoteObjectNodePrivate;
};

QT_END_NAMESPACE

#endif
