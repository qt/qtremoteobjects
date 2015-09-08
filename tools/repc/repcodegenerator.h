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

#ifndef REPCODEGENERATOR_H
#define REPCODEGENERATOR_H

#include <QSet>
#include <QString>
#include <QVector>

struct AST;
struct ASTClass;
struct POD;

QT_BEGIN_NAMESPACE
class QIODevice;
class QStringList;
class QTextStream;
QT_END_NAMESPACE

class RepCodeGenerator
{
public:
    enum Mode
    {
        REPLICA,
        SOURCE,
        SIMPLE_SOURCE,
        MERGED
    };

    explicit RepCodeGenerator(QIODevice *outputDevice);

    void generate(const AST &ast, Mode mode, QString fileName);

private:
    void generateHeader(Mode mode, QTextStream &out, const AST &ast);
    QString generateMetaTypeRegistration(const QSet<QString> &metaTypes);
    QString generateMetaTypeRegistrationForEnums(const QVector<QString> &enums);
    void generateStreamOperatorsForEnums(QTextStream &out, const QVector<QString> &enums);

    void generatePOD(QTextStream &out, const POD &pod);
    QString formatQPropertyDeclarations(const POD &pod);
    QString formatConstructors(const POD &pod);
    QString formatPropertyGettersAndSetters(const POD &pod);
    QString formatSignals(const POD &pod);
    QString formatDataMembers(const POD &pod);
    QString formatMarshallingOperators(const POD &pod);

    void generateClass(Mode mode, QStringList &out, const ASTClass &astClasses, const QString &metaTypeRegistrationCode);
    void generateSourceAPI(QStringList &out, const ASTClass &astClass);

private:
    QIODevice *m_outputDevice;
};

#endif
