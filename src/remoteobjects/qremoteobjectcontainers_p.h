/****************************************************************************
**
** Copyright (C) 2021 Ford Motor Company
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

#pragma once

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

#include <QtCore/qassociativeiterable.h>
#include <QtCore/qsequentialiterable.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QtROSequentialContainer : public QVariantList
{
public:
    QtROSequentialContainer() = default;
    QtROSequentialContainer(const QSequentialIterable &lst)
    {
        m_valueType = lst.metaContainer().valueMetaType();
        reserve(lst.size());
        for (const auto v : lst)
            append(v);
    }
    void setValueType(const QByteArray &valueTypeName)
    {
        m_valueTypeName = valueTypeName;
        m_valueType = QMetaType::fromName(valueTypeName.constData());
        clear();
    }

    QMetaType m_valueType;
    QByteArray m_typeName, m_valueTypeName;
};

QDataStream &operator>>(QDataStream &ds, QtROSequentialContainer &p);

QDataStream &operator<<(QDataStream &ds, const QtROSequentialContainer &p);

class QtROAssociativeContainer : public QVariantMap
{
public:
    QtROAssociativeContainer() = default;
    QtROAssociativeContainer(const QAssociativeIterable &map)
    {
        m_keyType = map.metaContainer().keyMetaType();
        m_valueType = map.metaContainer().mappedMetaType();
        m_keys.reserve(map.size());
        QAssociativeIterable::const_iterator iter = map.begin();
        const QAssociativeIterable::const_iterator end = map.end();
        for ( ; iter != end; ++iter) {
            m_keys.append(iter.key());
            insert(iter.key().toString(), iter.value());
        }
    }
    void setTypes(const QByteArray &keyTypeName, const QByteArray &valueTypeName)
    {
        m_keyTypeName = keyTypeName;
        m_keyType = QMetaType::fromName(keyTypeName.constData());
        m_valueTypeName = valueTypeName;
        m_valueType = QMetaType::fromName(valueTypeName.constData());
        clear();
        m_keys.clear();
    }

    QMetaType m_keyType, m_valueType;
    QByteArray m_typeName, m_keyTypeName, m_valueTypeName;
    QVariantList m_keys; // Map uses QString for key, this doesn't lose information for proxy
};

QDataStream &operator>>(QDataStream &ds, QtROAssociativeContainer &p);

QDataStream &operator<<(QDataStream &ds, const QtROAssociativeContainer &p);

QT_END_NAMESPACE
