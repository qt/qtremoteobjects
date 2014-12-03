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

#ifndef QREMOTEOBJECTSOURCE_P_H
#define QREMOTEOBJECTSOURCE_P_H

#include <QObject>
#include <QMetaObject>
#include <QMetaProperty>
#include <QVector>

QT_BEGIN_NAMESPACE

class ServerIoDevice;

template <typename Container>
class ContainerWithOffset {
public:
    typedef typename Container::value_type value_type;
    typedef typename Container::reference reference;
    typedef typename Container::const_reference const_reference;
    typedef typename Container::difference_type difference_type;
    typedef typename Container::pointer pointer;
    typedef typename Container::const_pointer const_pointer;
    typedef typename Container::iterator iterator;
    typedef typename Container::const_iterator const_iterator;

    ContainerWithOffset() :c(), offset(0) {}
    ContainerWithOffset(off_t offset)
        : c(), offset(offset) {}
    ContainerWithOffset(const Container &c, off_t offset)
        : c(c), offset(offset) {}
#ifdef Q_COMPILER_RVALUE_REFS
    ContainerWithOffset(Container &&c, off_t offset)
        : c(std::move(c)), offset(offset) {}
#endif

    reference operator[](int i) { return c[i-offset]; };
    const_reference operator[](int i) const { return c[i-offset]; }

    void reserve(int size) { c.reserve(size); }
    void push_back(const value_type &v) { c.push_back(v); }
#ifdef Q_COMPILER_RVALUE_REFS
    void push_back(value_type &&v) { c.push_back(std::move(v)); }
#endif

private:
    Container c;
    off_t offset;
};

class QRemoteObjectSourcePrivate : public QObject
{
public:
    explicit QRemoteObjectSourcePrivate(QObject *object, QMetaObject const *meta, const QString &name);

    ~QRemoteObjectSourcePrivate();

    int qt_metacall(QMetaObject::Call call, int methodId, void **a);
    ContainerWithOffset<QVector<QVector<int> > > args;
    QHash<int, QMetaProperty> propertyFromNotifyIndex;
    QList<ServerIoDevice*> listeners;
    QString m_name;
    QObject *m_object;
    const QMetaObject * const m_meta;
    const int m_methodOffset;
    const int m_propertyOffset;

    QVariantList marshalArgs(int index, void **a);
    void handleMetaCall(int index, QMetaObject::Call call, void **a);
    void addListener(ServerIoDevice *io, bool dynamic = false);
    int removeListener(ServerIoDevice *io, bool shouldSendRemove = false);
    bool invoke(QMetaObject::Call c, int index, const QVariantList& args, QVariant* returnValue = Q_NULLPTR);
};

QT_END_NAMESPACE

#endif
