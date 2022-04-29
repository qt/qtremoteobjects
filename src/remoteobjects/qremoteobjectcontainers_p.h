/****************************************************************************
**
** Copyright (C) 2021 Ford Motor Company
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
