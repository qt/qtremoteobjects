/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QDYNAMICREMOTEOBJECT_H
#define QDYNAMICREMOTEOBJECT_H

#include <QtRemoteObjects/qremoteobjectreplica.h>

QT_BEGIN_NAMESPACE

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectDynamicReplica : public QRemoteObjectReplica
{
public:
    ~QRemoteObjectDynamicReplica() override;

    const QMetaObject *metaObject() const override;
    void *qt_metacast(const char *name) override;
    int qt_metacall(QMetaObject::Call call, int id, void **argv) override;

private:
    explicit QRemoteObjectDynamicReplica();
    explicit QRemoteObjectDynamicReplica(QRemoteObjectNode *node, const QString &name);
    friend class QRemoteObjectNodePrivate;
    friend class QRemoteObjectNode;
};

QT_END_NAMESPACE

#endif
