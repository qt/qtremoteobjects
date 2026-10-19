// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QObject>

class SimpleObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
public:
    using QObject::QObject;

    int value() const { return m_value; }
    void setValue(int value)
    {
        if (value != m_value) {
            m_value = value;
            emit valueChanged(value);
        }
    }

Q_SIGNALS:
    void valueChanged(int value);

private:
    int m_value = 0;
};
