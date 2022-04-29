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

#ifndef QREGISTRYSOURCE_P_H
#define QREGISTRYSOURCE_P_H

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

#include "qtremoteobjectglobal.h"

QT_BEGIN_NAMESPACE

class QRegistrySource : public QObject
{
    Q_OBJECT
    Q_CLASSINFO(QCLASSINFO_REMOTEOBJECT_TYPE, "Registry")

    Q_PROPERTY(QRemoteObjectSourceLocations sourceLocations READ sourceLocations)

public:
    explicit QRegistrySource(QObject *parent = nullptr);
    ~QRegistrySource() override;

    QRemoteObjectSourceLocations sourceLocations() const;

Q_SIGNALS:
    void remoteObjectAdded(const QRemoteObjectSourceLocation &entry);
    void remoteObjectRemoved(const QRemoteObjectSourceLocation &entry);

public Q_SLOTS:
    void addSource(const QRemoteObjectSourceLocation &entry);
    void removeSource(const QRemoteObjectSourceLocation &entry);
    void removeServer(const QUrl &url);

private:
    QRemoteObjectSourceLocations m_sourceLocations;
};

QT_END_NAMESPACE

#endif
