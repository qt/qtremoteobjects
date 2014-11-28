/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB)
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

#include "repparser.h"

#include <QTemporaryFile>
#include <QTest>
#include <QTextStream>
#include <QDebug>

Q_DECLARE_METATYPE(ASTProperty::Modifier)

class tst_Parser : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testProperties_data();
    void testProperties();
    void testSlots_data();
    void testSlots();
    void testSignals_data();
    void testSignals();
    void testPods_data();
    void testPods();
};


void tst_Parser::testProperties_data()
{
    QTest::addColumn<QString>("propertyDeclaration");
    QTest::addColumn<QString>("expectedType");
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<QString>("expectedDefaultValue");
    QTest::addColumn<ASTProperty::Modifier>("expectedModifier");

    QTest::newRow("default") << "PROP(QString foo)" << "QString" << "foo" << QString() << ASTProperty::ReadWrite;
    QTest::newRow("readonly") << "PROP(QString foo READONLY)" << "QString" << "foo" << QString() << ASTProperty::ReadOnly;
    QTest::newRow("constant") << "PROP(QString foo CONSTANT)" << "QString" << "foo" << QString() << ASTProperty::Constant;
    QTest::newRow("defaultWithValue") << "PROP(int foo=1)" << "int" << "foo" << "1" << ASTProperty::ReadWrite;
    QTest::newRow("readonlyWithValue") << "PROP(int foo=1 READONLY)" << "int" << "foo" << "1" << ASTProperty::ReadOnly;
    QTest::newRow("constantWithValue") << "PROP(int foo=1 CONSTANT)" << "int" << "foo" << "1" << ASTProperty::Constant;
    QTest::newRow("defaultWhitespaces") << "PROP(  QString   foo  )" << "QString" << "foo" << QString() << ASTProperty::ReadWrite;
    QTest::newRow("defaultWhitespacesBeforeParentheses") << "PROP     (  QString   foo  )" << "QString" << "foo" << QString() << ASTProperty::ReadWrite;
    QTest::newRow("readonlyWhitespaces") << "PROP(  QString   foo   READONLY  )" << "QString" << "foo" << QString() << ASTProperty::ReadOnly;
    QTest::newRow("constantWhitespaces") << "PROP(  QString   foo   CONSTANT  )" << "QString" << "foo" << QString() << ASTProperty::Constant;
    QTest::newRow("defaultWithValueWhitespaces") << "PROP(  int foo  = 1 )" << "int" << "foo" << "1" << ASTProperty::ReadWrite;
    QTest::newRow("readonlyWithValueWhitespaces") << "PROP(  int foo = 1 READONLY  )" << "int" << "foo" << "1" << ASTProperty::ReadOnly;
    QTest::newRow("constantWithValueWhitespaces") << "PROP(  int foo = 1 CONSTANT )" << "int" << "foo" << "1" << ASTProperty::Constant;
    QTest::newRow("templatetype") << "PROP(QVector<int> bar)" << "QVector<int>" << "bar" << QString() << ASTProperty::ReadWrite;
    QTest::newRow("nested templatetype") << "PROP(QMap<int, QVector<int> > bar)" << "QMap<int, QVector<int> >" << "bar" << QString() << ASTProperty::ReadWrite;
    QTest::newRow("non-int default value") << "PROP(double foo=1.1 CONSTANT)" << "double" << "foo" << "1.1" << ASTProperty::Constant;
}

void tst_Parser::testProperties()
{
    QFETCH(QString, propertyDeclaration);
    QFETCH(QString, expectedType);
    QFETCH(QString, expectedName);
    QFETCH(QString, expectedDefaultValue);
    QFETCH(ASTProperty::Modifier, expectedModifier);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << "class TestClass" << endl;
    stream << "{" << endl;
    stream << propertyDeclaration << endl;
    stream << "};" << endl;
    file.close();

    RepParser parser(file.fileName());
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.count(), 1);

    const ASTClass astClass = ast.classes.first();
    const QVector<ASTProperty> properties = astClass.properties;
    QCOMPARE(properties.count(), 1);

    const ASTProperty property = properties.first();
    QCOMPARE(property.type, expectedType);
    QCOMPARE(property.name, expectedName);
    QCOMPARE(property.defaultValue, expectedDefaultValue);
    QCOMPARE(property.modifier, expectedModifier);
}

void tst_Parser::testSlots_data()
{
    QTest::addColumn<QString>("slotDeclaration");
    QTest::addColumn<QString>("expectedSlot");
    QTest::newRow("slotwithoutspacebeforeparentheses") << "SLOT(test())" << "void test()";
    QTest::newRow("slotwithspacebeforeparentheses") << "SLOT (test())" << "void test()";
    QTest::newRow("slotwitharguments") << "SLOT(void test(QString value, int number))" << "void test(QString value, int number)";
    QTest::newRow("slotwithunnamedarguments") << "SLOT(void test(QString, int))" << "void test(QString __repc_variable_1, int __repc_variable_2)";
    QTest::newRow("slotwithspaces") << "SLOT(   void test  (QString value, int number)  )" << "void test(QString value, int number)";
    QTest::newRow("slotwithtemplates") << "SLOT(test(QMap<QString,int> foo))" << "void test(QMap<QString,int> foo)";
    QTest::newRow("slotwithmultitemplates") << "SLOT(test(QMap<QString,int> foo, QMap<QString,int> bla))" << "void test(QMap<QString,int> foo, QMap<QString,int> bla)";
    QTest::newRow("slotwithtemplatetemplates") << "SLOT(test(QMap<QList<QString>,int> foo))" << "void test(QMap<QList<QString>,int> foo)";
    QTest::newRow("slotwithtemplateswithspace") << "SLOT ( test (QMap<QString , int>  foo ) )" << "void test(QMap<QString , int> foo)";
    QTest::newRow("slotWithConstRefArgument") << "SLOT (test(const QString &val))" << "void test(const QString & val)";
    QTest::newRow("slotWithRefArgument") << "SLOT (test(QString &val))" << "void test(QString & val)";
    QTest::newRow("slotwithtemplatetemplatesAndConstRef") << "SLOT(test(const QMap<QList<QString>,int> &foo))" << "void test(const QMap<QList<QString>,int> & foo)";
}

void tst_Parser::testSlots()
{
    QFETCH(QString, slotDeclaration);
    QFETCH(QString, expectedSlot);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << "class TestClass" << endl;
    stream << "{" << endl;
    stream << slotDeclaration << endl;
    stream << "};" << endl;
    file.close();

    RepParser parser(file.fileName());
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.count(), 1);

    const ASTClass astClass = ast.classes.first();
    const QVector<ASTFunction> slotsList = astClass.slotsList;
    QCOMPARE(slotsList.count(), 1);
    ASTFunction slot = slotsList.first();
    QCOMPARE(QString("%1 %2(%3)").arg(slot.returnType).arg(slot.name).arg(slot.paramsAsString()), expectedSlot);
}

void tst_Parser::testSignals_data()
{
    QTest::addColumn<QString>("signalDeclaration");
    QTest::addColumn<QString>("expectedSignal");
    QTest::newRow("signalwithoutspacebeforeparentheses") << "SIGNAL(test())" << "test()";
    QTest::newRow("signalwithspacebeforeparentheses") << "SIGNAL (test())" << "test()";
    QTest::newRow("signalwitharguments") << "SIGNAL(test(QString value, int value))" << "test(QString value, int value)";
    QTest::newRow("signalwithtemplates") << "SIGNAL(test(QMap<QString,int> foo))" << "test(QMap<QString,int> foo)";
    QTest::newRow("signalwithtemplateswithspace") << "SIGNAL ( test (QMap<QString , int>  foo ) )" << "test(QMap<QString , int> foo)";
    QTest::newRow("signalWithConstRefArgument") << "SIGNAL (test(const QString &val))" << "test(const QString & val)";
    QTest::newRow("signalWithRefArgument") << "SIGNAL (test(QString &val))" << "test(QString & val)";
}

void tst_Parser::testSignals()
{
    QFETCH(QString, signalDeclaration);
    QFETCH(QString, expectedSignal);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << "class TestClass" << endl;
    stream << "{" << endl;
    stream << signalDeclaration << endl;
    stream << "};" << endl;
    file.close();

    RepParser parser(file.fileName());
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.count(), 1);

    const ASTClass astClass = ast.classes.first();
    const QVector<ASTFunction> signalsList = astClass.signalsList;
    ASTFunction signal = signalsList.first();
    QCOMPARE(QString("%1(%2)").arg(signal.name).arg(signal.paramsAsString()), expectedSignal);
}

void tst_Parser::testPods_data()
{
    QTest::addColumn<QString>("podsdeclaration");
    QTest::addColumn<QString>("expectedtypes");
    QTest::addColumn<QString>("expectedvariables");

    //Variable/Type separate by ";"
    QTest::newRow("one pod") << "POD preset(int presetNumber)" << "int" << "presetNumber";
    QTest::newRow("two pod") << "POD preset(int presetNumber, double foo)" << "int;double" << "presetNumber;foo";
    QTest::newRow("two pod with space") << "POD preset ( int presetNumber , double foo ) " << "int;double" << "presetNumber;foo";
    //Template
    QTest::newRow("pod template") << "POD preset(QMap<QString,int> foo) " << "QMap<QString,int>" << "foo";
    QTest::newRow("pod template (QList)") << "POD preset(QList<QString> foo) " << "QList<QString>" << "foo";
    QTest::newRow("two pod template") << "POD preset(QMap<QString,int> foo, QMap<double,int> bla) " << "QMap<QString,int>;QMap<double,int>" << "foo;bla";
    QTest::newRow("two pod template with space") << "POD preset( QMap<QString  ,  int >  foo ,   QMap<  double , int > bla ) " << "QMap<QString  ,  int >;QMap<  double , int >" << "foo;bla";

}

void tst_Parser::testPods()
{
    QFETCH(QString, podsdeclaration);
    QFETCH(QString, expectedtypes);
    QFETCH(QString, expectedvariables);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << podsdeclaration << endl;
    stream << "class TestClass" << endl;
    stream << "{" << endl;
    stream << "};" << endl;
    file.close();

    RepParser parser(file.fileName());
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.count(), 1);

    QCOMPARE(ast.pods.count(), 1);
    const POD pods = ast.pods.first();
    const QVector<PODAttribute> podsList = pods.attributes;
    const QStringList typeList = expectedtypes.split(QLatin1Char(';'));
    const QStringList variableList = expectedvariables.split(QLatin1Char(';'));
    QVERIFY(typeList.count() == variableList.count());
    QVERIFY(podsList.count() == variableList.count());
    for (int i=0; i < podsList.count(); ++i) {
        QCOMPARE(podsList.at(i).name, variableList.at(i));
        QCOMPARE(podsList.at(i).type, typeList.at(i));
    }
}


QTEST_APPLESS_MAIN(tst_Parser)

#include "tst_parser.moc"
