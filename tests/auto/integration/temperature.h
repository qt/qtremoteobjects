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

#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <QString>

class Temperature
{
public:
    Temperature() : _value(0) {}
    Temperature(double value, const QString &unit) : _value(value), _unit(unit) {}

    void setValue(double arg) { _value = arg; }
    double value() const { return _value; }

    void setUnit(const QString &arg) { _unit = arg; }
    QString unit() const { return _unit; }

private:
    double _value;
    QString _unit;
};

inline bool operator==(const Temperature &lhs, const Temperature &rhs)
{
    return lhs.unit() == rhs.unit() &&
            lhs.value() == rhs.value();
}

inline bool operator!=(const Temperature &lhs, const Temperature &rhs)
{
    return !(lhs == rhs);
}

inline QDataStream &operator<<(QDataStream &out, const Temperature &temperature)
{
    out << temperature.value();
    out << temperature.unit();
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Temperature &temperature)
{
    double value;
    in >> value;
    temperature.setValue(value);

    QString unit;
    in >> unit;
    temperature.setUnit(unit);
    return in;
}

Q_DECLARE_METATYPE(Temperature);

#endif
