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

#include "qremoteobjectregistrysource_p.h"
#include <QDataStream>

QT_BEGIN_NAMESPACE

QRegistrySource::QRegistrySource(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaTypeStreamOperators<QRemoteObjectSourceLocation>();
    qRegisterMetaTypeStreamOperators<QRemoteObjectSourceLocations>();
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
    QVector<QString> results;
    typedef QRemoteObjectSourceLocations::const_iterator CustomIterator;
    const CustomIterator end = m_sourceLocations.constEnd();
    for (CustomIterator it = m_sourceLocations.constBegin(); it != end; ++it)
        if (it.value().hostUrl == url)
            results.push_back(it.key());
    Q_FOREACH (const QString &res, results)
        m_sourceLocations.remove(res);
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
