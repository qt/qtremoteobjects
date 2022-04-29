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

#include "qremoteobjectregistrysource_p.h"
#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

QRegistrySource::QRegistrySource(QObject *parent)
    : QObject(parent)
{
}

QRegistrySource::~QRegistrySource()
{
}

QRemoteObjectSourceLocations QRegistrySource::sourceLocations() const
{
    qCDebug(QT_REMOTEOBJECT) << "sourceLocations property requested on RegistrySource" << m_sourceLocations;
    return m_sourceLocations;
}

void QRegistrySource::removeServer(const QUrl &url)
{
    for (auto it = m_sourceLocations.begin(), end = m_sourceLocations.end(); it != end; /* erasing */) {
        if (it.value().hostUrl == url)
            it = m_sourceLocations.erase(it);
        else
            ++it;
    }
}

void QRegistrySource::addSource(const QRemoteObjectSourceLocation &entry)
{
    qCDebug(QT_REMOTEOBJECT) << "An entry was added to the RegistrySource" << entry;
    if (m_sourceLocations.contains(entry.first)) {
        if (m_sourceLocations[entry.first].hostUrl == entry.second.hostUrl)
            qCWarning(QT_REMOTEOBJECT) << "Node warning: Ignoring Source" << entry.first
                                       << "as this Node already has a Source by that name.";
        else
            qCWarning(QT_REMOTEOBJECT) << "Node warning: Ignoring Source" << entry.first
                                       << "as another source (" << m_sourceLocations[entry.first]
                                       << ") has already registered that name.";
        return;
    }
    m_sourceLocations[entry.first] = entry.second;
    emit remoteObjectAdded(entry);
}

void QRegistrySource::removeSource(const QRemoteObjectSourceLocation &entry)
{
    if (m_sourceLocations.contains(entry.first) && m_sourceLocations[entry.first].hostUrl == entry.second.hostUrl) {
        m_sourceLocations.remove(entry.first);
        emit remoteObjectRemoved(entry);
    }
}

QT_END_NAMESPACE
