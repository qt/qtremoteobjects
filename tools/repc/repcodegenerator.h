/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef REPCODEGENERATOR_H
#define REPCODEGENERATOR_H

#include "repparser.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QTextStream>

QT_BEGIN_NAMESPACE

class QIODevice;

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

    RepCodeGenerator(QIODevice *outputDevice, const AST &ast);

    void generate(Mode mode, QString fileName);

    QByteArray classSignature(const ASTClass &ac);
private:
    void generateHeader(Mode mode);
    QString generateMetaTypeRegistration(const QSet<QString> &metaTypes);
    QString generateMetaTypeRegistrationForPending(const QSet<QString> &metaTypes);

    void generateSimpleSetter(const ASTProperty &property, bool generateOverride = true);
    void generatePOD(const POD &pod);
    void generateEnumGadget(const ASTEnum &en, const QString &className);
    void generateDeclarationsForEnums(const QList<ASTEnum> &enums, bool generateQENUM=true);
    QString formatQPropertyDeclarations(const POD &pod);
    QString formatConstructors(const POD &pod);
    QString formatPropertyGettersAndSetters(const POD &pod);
    QString formatSignals(const POD &pod);
    QString formatDataMembers(const POD &pod);
    QString formatDebugOperator(const POD &pod);
    QString formatMarshallingOperators(const POD &pod);
    QString typeForMode(const ASTProperty &property, Mode mode);

    void generateClass(Mode mode, const ASTClass &astClasses,
                       const QString &metaTypeRegistrationCode);
    void generateSourceAPI(const ASTClass &astClass);

private:
    QTextStream m_stream;
    AST m_ast;
};

QT_END_NAMESPACE

#endif
