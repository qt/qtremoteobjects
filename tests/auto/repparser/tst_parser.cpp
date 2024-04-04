// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "repparser.h"

#include <QTemporaryFile>
#include <QTest>
#include <QTextStream>

Q_DECLARE_METATYPE(ASTProperty::Modifier)
Q_DECLARE_METATYPE(ASTModelRole)

class tst_Parser : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testBasic_data();
    void testBasic();
    void testProperties_data();
    void testProperties();
    void testSlots_data();
    void testSlots();
    void testSignals_data();
    void testSignals();
    void testPods_data();
    void testPods();
    void testPods2_data();
    void testPods2();
    void testCompilerAttributes();
    void testEnums_data();
    void testEnums();
    void testTypedEnums_data();
    void testTypedEnums();
    void testModels_data();
    void testModels();
    void testClasses_data();
    void testClasses();
    void testInvalid_data();
    void testInvalid();
};

void tst_Parser::testBasic_data()
{
    QTest::addColumn<QString>("content");

    //Comment out "empty" tests that fail QLALR parser...
    //QTest::newRow("empty") << ""; // empty lines are fine...
    QTest::newRow("preprocessor_line_include") << "#include \"foo\"";
    QTest::newRow("preprocessor_line_include_spaces") << "#  include \"foo\"";
    QTest::newRow("preprocessor_line_ifgroup") << "#if 1\n#include \"foo\n#endif";
    //QTest::newRow("comment") << "//This is a comment";
    QTest::newRow("enum") << "ENUM MyEnum {test}";
    QTest::newRow("empty class with comment") << "class MyClass {\n//comment\n}";
    QTest::newRow("empty exported class with comment") << "class Q_DECL_EXPORT MyClass {\n//comment\n}";
    QTest::newRow("comment, class") << "//comment\nclass MyClass {}";
    QTest::newRow("multicomment, class") << "/* row1\n row2\n */\nclass MyClass {}";
    QTest::newRow("include, comment, class") << "#include \"foo\"\n//comment\nclass MyClass {}";
}

void tst_Parser::testBasic()
{
    QFETCH(QString, content);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << content << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());
}

void tst_Parser::testProperties_data()
{
    QTest::addColumn<QString>("propertyDeclaration");
    QTest::addColumn<QString>("expectedType");
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<QString>("expectedDefaultValue");
    QTest::addColumn<ASTProperty::Modifier>("expectedModifier");
    QTest::addColumn<bool>("expectedPersistence");

    QTest::newRow("default") << "PROP(QString foo)" << "QString" << "foo" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("default with comment") << "PROP(QString foo) // my property" << "QString" << "foo" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("default with comment above") << "// my property\nPROP(QString foo)" << "QString" << "foo" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("default with indented comment above") << "    // my property\nPROP(QString foo)" << "QString" << "foo" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("readonly") << "PROP(QString foo READONLY)" << "QString" << "foo" << QString() << ASTProperty::ReadOnly << false;
    QTest::newRow("constant") << "PROP(QString foo CONSTANT)" << "QString" << "foo" << QString() << ASTProperty::Constant << false;
    QTest::newRow("readwrite") << "PROP(QString foo READWRITE)" << "QString" << "foo" << QString() << ASTProperty::ReadWrite << false;
    QTest::newRow("readpush") << "PROP(QString foo READPUSH)" << "QString" << "foo" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("persisted") << "PROP(QString foo PERSISTED)" << "QString" << "foo" << QString() << ASTProperty::ReadPush << true;
    QTest::newRow("readonly, persisted") << "PROP(QString foo READONLY, PERSISTED)" << "QString" << "foo" << QString() << ASTProperty::ReadOnly << true;
    QTest::newRow("readwrite, persisted") << "PROP(QString foo READWRITE, PERSISTED)" << "QString" << "foo" << QString() << ASTProperty::ReadWrite << true;
    QTest::newRow("readpush, persisted") << "PROP(QString foo READPUSH, PERSISTED)" << "QString" << "foo" << QString() << ASTProperty::ReadPush << true;
    QTest::newRow("constant,persisted") << "PROP(QString foo CONSTANT, PERSISTED)" << "QString" << "foo" << QString() << ASTProperty::Constant << true;
    QTest::newRow("constant,readonly,persisted") << "PROP(QString foo CONSTANT, READONLY, PERSISTED)" << "QString" << "foo" << QString() << ASTProperty::Constant << true;
    QTest::newRow("readonly,constant,persisted") << "PROP(QString foo READONLY,CONSTANT, PERSISTED)" << "QString" << "foo" << QString() << ASTProperty::Constant << true;
    QTest::newRow("defaultWithValue") << "PROP(int foo=1)" << "int" << "foo" << "1" << ASTProperty::ReadPush << false;
    QTest::newRow("readonlyWithValue") << "PROP(int foo=1 READONLY)" << "int" << "foo" << "1" << ASTProperty::ReadOnly << false;
    QTest::newRow("constantWithValue") << "PROP(int foo=1 CONSTANT)" << "int" << "foo" << "1" << ASTProperty::Constant << false;
    QTest::newRow("defaultWhitespaces") << "PROP(  QString   foo  )" << "QString" << "foo" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("defaultWhitespacesBeforeParentheses") << "PROP     (  QString   foo  )" << "QString" << "foo" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("readonlyWhitespaces") << "PROP(  QString   foo   READONLY  )" << "QString" << "foo" << QString() << ASTProperty::ReadOnly << false;
    QTest::newRow("constantWhitespaces") << "PROP(  QString   foo   CONSTANT  )" << "QString" << "foo" << QString() << ASTProperty::Constant << false;
    QTest::newRow("defaultWithValueWhitespaces") << "PROP(  int foo  = 1 )" << "int" << "foo" << "1" << ASTProperty::ReadPush << false;
    QTest::newRow("readonlyWithValueWhitespaces") << "PROP(  int foo = 1 READONLY  )" << "int" << "foo" << "1" << ASTProperty::ReadOnly << false;
    QTest::newRow("constantWithValueWhitespaces") << "PROP(  int foo = 1 CONSTANT )" << "int" << "foo" << "1" << ASTProperty::Constant << false;
    QTest::newRow("templatetype") << "PROP(QList<int> bar)" << "QList<int>" << "bar" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("nested templatetype") << "PROP(QMap<int, QList<int>> bar)" << "QMap<int, QList<int>>" << "bar" << QString() << ASTProperty::ReadPush << false;
    QTest::newRow("non-int default value") << "PROP(double foo=1.1 CONSTANT)" << "double" << "foo" << "1.1" << ASTProperty::Constant << false;
    QTest::newRow("tab") << "PROP(double\tfoo)" << "double" << "foo" << "" << ASTProperty::ReadPush << false;
    QTest::newRow("two tabs") << "PROP(double\t\tfoo)" << "double" << "foo" << "" << ASTProperty::ReadPush << false;
    QTest::newRow("stringWithValue") << "PROP(QString foo=\"Hello World\")" << "QString" << "foo"
                                     << QString("\"Hello World\"") << ASTProperty::ReadPush << false;
    QTest::newRow("stringWithValueWhitespaces") << "PROP(QString foo   =   \"Hello   World\")"
                                                << "QString" << "foo" << QString("\"Hello   World\"")
                                                << ASTProperty::ReadPush << false;
    QTest::newRow("readonlyStringWithValueWhitespaces") << "PROP(QString foo   =   \"Hello   World\"   READONLY  )"
                                                        << "QString" << "foo" << "\"Hello   World\""
                                                        << ASTProperty::ReadOnly << false;
    QTest::newRow("readonlyStringWithValue") << "PROP(QString foo=\"Hello World\" READONLY)"
                                             << "QString" << "foo" << "\"Hello World\""
                                             << ASTProperty::ReadOnly << false;
}

void tst_Parser::testProperties()
{
    QFETCH(QString, propertyDeclaration);
    QFETCH(QString, expectedType);
    QFETCH(QString, expectedName);
    QFETCH(QString, expectedDefaultValue);
    QFETCH(ASTProperty::Modifier, expectedModifier);
    QFETCH(bool, expectedPersistence);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << "class TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << propertyDeclaration << Qt::endl;
    stream << "};" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 1);

    const ASTClass astClass = ast.classes.first();
    const QList<ASTProperty> properties = astClass.properties;
    QCOMPARE(properties.size(), 1);

    const ASTProperty property = properties.first();
    QCOMPARE(property.type, expectedType);
    QCOMPARE(property.name, expectedName);
    QCOMPARE(property.defaultValue, expectedDefaultValue);
    QCOMPARE(property.modifier, expectedModifier);
    QCOMPARE(property.persisted, expectedPersistence);
}

void tst_Parser::testSlots_data()
{
    QTest::addColumn<QString>("slotDeclaration");
    QTest::addColumn<QString>("expectedSlot");
    QTest::addColumn<bool>("voidWarning");
    QTest::newRow("slotwithoutspacebeforeparentheses") << "SLOT(test())" << "void test()" << true;
    QTest::newRow("slotwithoutspacebeforeparentheses with comment") << "SLOT(test()) // my slot" << "void test()" << true;
    QTest::newRow("slotwithoutspacebeforeparentheses with comment above") << "// my slot\nSLOT(test())" << "void test()" << true;
    QTest::newRow("slotwithoutspacebeforeparentheses with indented comment above") << "    // my slot\nSLOT(test())" << "void test()" << true;
    QTest::newRow("slotwithspacebeforeparentheses") << "SLOT (test())" << "void test()" << true;
    QTest::newRow("slotwitharguments") << "SLOT(void test(QString value, int number))" << "void test(QString value, int number)" << false;
    QTest::newRow("slotwithunnamedarguments") << "SLOT(void test(QString, int))" << "void test(QString __repc_variable_1, int __repc_variable_2)" << false;
    QTest::newRow("slotwithspaces") << "SLOT(   void test  (QString value, int number)  )" << "void test(QString value, int number)" << false;
    QTest::newRow("slotwithtemplates") << "SLOT(test(QMap<QString,int> foo))" << "void test(QMap<QString,int> foo)" << true;
    QTest::newRow("slotwithmultitemplates") << "SLOT(test(QMap<QString,int> foo, QMap<QString,int> bla))" << "void test(QMap<QString,int> foo, QMap<QString,int> bla)" << true;
    QTest::newRow("slotwithtemplatetemplates") << "SLOT(test(QMap<QList<QString>,int> foo))" << "void test(QMap<QList<QString>,int> foo)" << true;
    QTest::newRow("slotwithtemplateswithspace") << "SLOT ( test (QMap<QString , int>  foo ) )" << "void test(QMap<QString , int> foo)" << true;
    QTest::newRow("slotWithConstRefArgument") << "SLOT (test(const QString &val))" << "void test(const QString & val)" << true;
    QTest::newRow("slotWithRefArgument") << "SLOT (test(QString &val))" << "void test(QString & val)" << true;
    QTest::newRow("slotwithtemplatetemplatesAndConstRef") << "SLOT(test(const QMap<QList<QString>,int> &foo))" << "void test(const QMap<QList<QString>,int> & foo)" << true;
    QTest::newRow("slotWithConstRefArgumentAndWithout") << "SLOT (test(const QString &val, int value))" << "void test(const QString & val, int value)" << true;
}

void tst_Parser::testSlots()
{
    QFETCH(QString, slotDeclaration);
    QFETCH(QString, expectedSlot);
    QFETCH(bool, voidWarning);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << "class TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << slotDeclaration << Qt::endl;
    stream << "};" << Qt::endl;
    file.seek(0);

    if (voidWarning)
        QTest::ignoreMessage(QtWarningMsg, "[repc] - Adding 'void' for unspecified return type on test");
    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 1);

    const ASTClass astClass = ast.classes.first();
    const QList<ASTFunction> slotsList = astClass.slotsList;
    QCOMPARE(slotsList.size(), 1);
    ASTFunction slot = slotsList.first();
    QCOMPARE(QString("%1 %2(%3)").arg(slot.returnType).arg(slot.name).arg(slot.paramsAsString()), expectedSlot);
}

void tst_Parser::testSignals_data()
{
    QTest::addColumn<QString>("signalDeclaration");
    QTest::addColumn<QString>("expectedSignal");
    QTest::newRow("signalwithoutspacebeforeparentheses") << "SIGNAL(test())" << "test()";
    QTest::newRow("signalwithoutspacebeforeparentheses with comment") << "SIGNAL(test()) // my signal" << "test()";
    QTest::newRow("signalwithoutspacebeforeparentheses with comment above") << "// my signal\nSIGNAL(test())" << "test()";
    QTest::newRow("signalwithoutspacebeforeparentheses with indented comment above") << "    // my signal\nSIGNAL(test())" << "test()";
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
    stream << "class TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << signalDeclaration << Qt::endl;
    stream << "};" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 1);

    const ASTClass astClass = ast.classes.first();
    const QList<ASTFunction> signalsList = astClass.signalsList;
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
    QTest::newRow("one pod with comment") << "POD preset(int presetNumber) // my pod" << "int" << "presetNumber";
    QTest::newRow("one pod with comment above") << "// my pod\nPOD preset(int presetNumber)" << "int" << "presetNumber";
    QTest::newRow("one pod with indented comment above") << "    // my pod\nPOD preset(int presetNumber)" << "int" << "presetNumber";
    QTest::newRow("two pod") << "POD preset(int presetNumber, double foo)" << "int;double" << "presetNumber;foo";
    QTest::newRow("two pod with space") << "POD preset ( int presetNumber , double foo ) " << "int;double" << "presetNumber;foo";
    QTest::newRow("two pod multiline") << "POD preset(\nint presetNumber,\ndouble foo\n)" << "int;double" << "presetNumber;foo";
    QTest::newRow("two pod multiline with comments") << "POD preset(// this is a pod\nint presetNumber, // presetNumber\ndouble foo // foo\n)" << "int;double" << "presetNumber;foo";
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
    stream << podsdeclaration << Qt::endl;
    stream << "class TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << "};" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 1);

    QCOMPARE(ast.pods.size(), 1);
    const POD pods = ast.pods.first();
    QVERIFY(pods.compilerAttribute.isEmpty());
    const QList<PODAttribute> podsList = pods.attributes;
    const QStringList typeList = expectedtypes.split(QLatin1Char(';'));
    const QStringList variableList = expectedvariables.split(QLatin1Char(';'));
    QVERIFY(typeList.size() == variableList.size());
    QVERIFY(podsList.size() == variableList.size());
    for (int i=0; i < podsList.size(); ++i) {
        QCOMPARE(podsList.at(i).name, variableList.at(i));
        QCOMPARE(podsList.at(i).type, typeList.at(i));
    }
}

void tst_Parser::testPods2_data()
{
    QTest::addColumn<QString>("podsdeclaration");
    QTest::addColumn<QString>("expectedtypes");
    QTest::addColumn<QString>("expectedvariables");

    //Variable/Type separate by ";"
    QTest::newRow("one pod") << "POD preset{int presetNumber}" << "int" << "presetNumber";
    QTest::newRow("one pod with comment") << "POD preset{int presetNumber} // my pod" << "int" << "presetNumber";
    QTest::newRow("one pod with comment above") << "// my pod\nPOD preset{int presetNumber}" << "int" << "presetNumber";
    QTest::newRow("one pod with indented comment above") << "    // my pod\nPOD preset{int presetNumber}" << "int" << "presetNumber";
    QTest::newRow("two pod") << "POD preset{int presetNumber, double foo}" << "int;double" << "presetNumber;foo";
    QTest::newRow("two pod with space") << "POD preset { int presetNumber , double foo } " << "int;double" << "presetNumber;foo";
    QTest::newRow("two pod multiline") << "POD preset{\nint presetNumber,\ndouble foo\n}" << "int;double" << "presetNumber;foo";
    //Template
    QTest::newRow("pod template") << "POD preset{QMap<QString,int> foo} " << "QMap<QString,int>" << "foo";
    QTest::newRow("pod template (QList)") << "POD preset{QList<QString> foo} " << "QList<QString>" << "foo";
    QTest::newRow("two pod template") << "POD preset{QMap<QString,int> foo, QMap<double,int> bla} " << "QMap<QString,int>;QMap<double,int>" << "foo;bla";
    QTest::newRow("two pod template with space") << "POD preset{ QMap<QString  ,  int >  foo ,   QMap<  double , int > bla } " << "QMap<QString,int>;QMap<double,int>" << "foo;bla";
    //Enum
    QTest::newRow("enum multiline") << "POD preset{ENUM val {val1 = 1,\nval2,\nval3=12}\nval value,\ndouble foo\n}" << "val;double" << "value;foo";

}

void tst_Parser::testPods2()
{
    QFETCH(QString, podsdeclaration);
    QFETCH(QString, expectedtypes);
    QFETCH(QString, expectedvariables);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << podsdeclaration << Qt::endl;
    stream << "class TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << "};" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 1);

    QCOMPARE(ast.pods.size(), 1);
    const POD pods = ast.pods.first();
    QVERIFY(pods.compilerAttribute.isEmpty());
    const QVector<PODAttribute> podsList = pods.attributes;
    const QStringList typeList = expectedtypes.split(QLatin1Char(';'));
    const QStringList variableList = expectedvariables.split(QLatin1Char(';'));
    QVERIFY(typeList.size() == variableList.size());
    QVERIFY(podsList.size() == variableList.size());
    for (int i=0; i < podsList.size(); ++i) {
        QCOMPARE(podsList.at(i).name, variableList.at(i));
        QCOMPARE(podsList.at(i).type, typeList.at(i));
    }
}

void tst_Parser::testCompilerAttributes()
{
    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << "POD Q_DECL_EXPORT TestPod(int number)" << Qt::endl;
    stream << "class Q_DECL_EXPORT TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << "};" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();

    QCOMPARE(ast.pods.size(), 1);
    const POD pods = ast.pods.first();
    QCOMPARE(pods.name, "TestPod");
    QCOMPARE(pods.compilerAttribute, "Q_DECL_EXPORT");

    QCOMPARE(ast.classes.size(), 1);
    const ASTClass klass = ast.classes.first();
    QCOMPARE(klass.name, "TestClass");
    QCOMPARE(klass.compilerAttribute, "Q_DECL_EXPORT");
}

void tst_Parser::testEnums_data()
{
    QTest::addColumn<QString>("enumdeclaration");
    QTest::addColumn<QString>("expectednames");
    QTest::addColumn<QList<int> >("expectedvalues");
    QTest::addColumn<int>("expectedmax");
    QTest::addColumn<bool>("expectedsigned");
    QTest::addColumn<bool>("inclass");

    for (int i = 0; i <= 1; ++i) {
        bool inclass = i == 1;
        QString identifier = inclass ? QLatin1String("%1 in class") : QLatin1String("%1 outside class");
        //Separate by ";"
        QTest::newRow(identifier.arg("one enum val").toLatin1()) << "ENUM preset {presetNumber}" << "presetNumber" << (QList<int>() << 0) << 0 << false << inclass;
        QTest::newRow(identifier.arg("one enum val with comment").toLatin1()) << "ENUM preset {presetNumber} // my enum" << "presetNumber" << (QList<int>() << 0) << 0 << false << inclass;
        QTest::newRow(identifier.arg("one enum val with comment above").toLatin1()) << "// my enum\nENUM preset {presetNumber}" << "presetNumber" << (QList<int>() << 0) << 0 << false << inclass;
        QTest::newRow(identifier.arg("one enum val with indented comment above").toLatin1()) << "    // my enum\nENUM preset {presetNumber}" << "presetNumber" << (QList<int>() << 0) << 0 << false << inclass;
        QTest::newRow(identifier.arg("two enum val").toLatin1()) << "ENUM preset {presetNumber, foo}" << "presetNumber;foo" << (QList<int>() << 0 << 1) << 1 << false << inclass;
        QTest::newRow(identifier.arg("two enum val -1 2nd").toLatin1()) << "ENUM preset {presetNumber, foo = -1}" << "presetNumber;foo" << (QList<int>() << 0 << -1) << 1 << true << inclass;
        QTest::newRow(identifier.arg("two enum val -1 1st").toLatin1()) << "ENUM preset {presetNumber=-1, foo}" << "presetNumber;foo" << (QList<int>() << -1 << 0) << 1 << true << inclass;
        QTest::newRow(identifier.arg("two enum val hex").toLatin1()) << "ENUM preset {presetNumber=0xf, foo}" << "presetNumber;foo" << (QList<int>() << 15 << 16) << 16 << false << inclass;
        QTest::newRow(identifier.arg("two enum val hex").toLatin1()) << "ENUM preset {presetNumber=0xff, foo}" << "presetNumber;foo" << (QList<int>() << 255 << 256) << 256 << false << inclass;
        QTest::newRow(identifier.arg("two enum val with space").toLatin1()) << "ENUM preset { presetNumber ,  foo } " << "presetNumber;foo" << (QList<int>() << 0 << 1) << 1 << false << inclass;
        QTest::newRow(identifier.arg("set values").toLatin1()) << "ENUM preset { val1=1 , val3=3, val5=5 } " << "val1;val3;val5" << (QList<int>() << 1 << 3 << 5) << 5 << false << inclass;
        QTest::newRow(identifier.arg("multiline").toLatin1()) << "ENUM preset {\nval1,\nval2,\nval3\n} " << "val1;val2;val3" << (QList<int>() << 0 << 1 << 2) << 2 << false << inclass;
        QTest::newRow(identifier.arg("multiline indented").toLatin1()) << "    ENUM preset {\n        val1,\n        val2,\n        val3\n    } " << "val1;val2;val3" << (QList<int>() << 0 << 1 << 2) << 2 << false << inclass;
    }
}

void tst_Parser::testEnums()
{
    QFETCH(QString, enumdeclaration);
    QFETCH(QString, expectednames);
    QFETCH(QList<int>, expectedvalues);
    QFETCH(int, expectedmax);
    QFETCH(bool, expectedsigned);
    QFETCH(bool, inclass);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    if (!inclass)
        stream << enumdeclaration << Qt::endl;
    stream << "class TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    if (inclass)
        stream << enumdeclaration << Qt::endl;
    stream << "};" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 1);
    ASTEnum enums;
    if (inclass) {
        const ASTClass astClass = ast.classes.first();
        QCOMPARE(astClass.enums.size(), 1);
        enums = astClass.enums.first();
    } else {
        QCOMPARE(ast.enums.size(), 1);
        enums = ast.enums.first();
    }
    QVERIFY(enums.isScoped == false);
    QVERIFY(enums.type.isEmpty());
    const QList<ASTEnumParam> paramList = enums.params;
    const QStringList nameList = expectednames.split(QLatin1Char(';'));
    QVERIFY(nameList.size() == expectedvalues.size());
    QVERIFY(paramList.size() == expectedvalues.size());
    for (int i=0; i < paramList.size(); ++i) {
        QCOMPARE(paramList.at(i).name, nameList.at(i));
        QCOMPARE(paramList.at(i).value, expectedvalues.at(i));
    }
    QCOMPARE(enums.max, expectedmax);
    QCOMPARE(enums.isSigned, expectedsigned);
}

void tst_Parser::testTypedEnums_data()
{
    QTest::addColumn<QString>("enumdeclaration");
    QTest::addColumn<QString>("expectedtype");
    QTest::addColumn<bool>("inclass");
    QTest::addColumn<bool>("isscoped");
    QTest::addColumn<bool>("isflag");

    for (int i = 0; i <= 7; ++i) {
        bool inclass = i % 2 == 1;
        bool isscoped = i % 4 > 1;
        bool isflag = i > 3;
        QString identifier = inclass ? QLatin1String("%1 %2 %3 in class") : QLatin1String("%1 %2 %3 outside class");
        QString scopeString = isscoped ? QLatin1String("Scoped") : QLatin1String("Non-scoped");
        QString flagString = isflag ? QLatin1String("Flag") : QLatin1String("Enum");
        QTest::newRow(identifier.arg(scopeString, flagString, "no type").toLatin1()) << "preset {presetNumber}" << QString() << inclass << isscoped << isflag;
        QTest::newRow(identifier.arg(scopeString, flagString, "quint16").toLatin1()) << "preset : quint16 {presetNumber}" << "quint16" << inclass << isscoped << isflag;
        QTest::newRow(identifier.arg(scopeString, flagString, "qint64").toLatin1()) << "preset : qint64 {presetNumber}" << "qint64" << inclass << isscoped << isflag;
        QTest::newRow(identifier.arg(scopeString, flagString, "unsigned char").toLatin1()) << "preset: unsigned char {presetNumber}" << "unsigned char" << inclass << isscoped << isflag;
    }
}

void tst_Parser::testTypedEnums()
{
    QFETCH(QString, enumdeclaration);
    QFETCH(QString, expectedtype);
    QFETCH(bool, inclass);
    QFETCH(bool, isscoped);
    QFETCH(bool, isflag);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    if (!inclass) {
        stream << " // comment 1" << Qt::endl;
        stream << "ENUM " << (isscoped ? "class " : "") << enumdeclaration
               << " // comment 2" << Qt::endl;
        if (isflag)
            stream << "//comment 3" << Qt::endl << "FLAG(MyFlags preset) // comment4" << Qt::endl;
    }
    stream << "class TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    if (inclass) {
        stream << " // comment" << Qt::endl;
        stream << "ENUM " << (isscoped ? "class " : "") << enumdeclaration
               << " // comment 2" << Qt::endl;
        if (isflag)
            stream << "//comment 3" << Qt::endl << "FLAG(MyFlags preset) // comment4" << Qt::endl;
    }
    stream << "};" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 1);
    ASTEnum enums;
    ASTFlag flags;
    if (inclass) {
        const ASTClass astClass = ast.classes.first();
        QCOMPARE(astClass.enums.size(), 1);
        enums = astClass.enums.first();
        if (isflag)
            flags = astClass.flags.first();
    } else {
        QCOMPARE(ast.enums.size(), 1);
        enums = ast.enums.first();
        if (isflag)
            flags = ast.flags.first();
    }
    QVERIFY(enums.isScoped == isscoped);
    QVERIFY(enums.flagIndex == (isflag ? 0 : -1));
    QVERIFY(flags.name == (isflag ? "MyFlags" : QString{}));
    QVERIFY(flags._enum == (isflag ? "preset" : QString{}));
    QCOMPARE(enums.type, expectedtype);
    const QList<ASTEnumParam> paramList = enums.params;
    QVERIFY(paramList.size() == 1);
    for (int i=0; i < paramList.size(); ++i) {
        QCOMPARE(paramList.at(i).name, "presetNumber");
        QCOMPARE(paramList.at(i).value, 0);
    }
    QCOMPARE(enums.max, 0);
    QCOMPARE(enums.isSigned, false);
}

void tst_Parser::testModels_data()
{
    QTest::addColumn<QString>("modelDeclaration");
    QTest::addColumn<QString>("expectedModel");
    QTest::addColumn<QList<ASTModelRole>>("expectedRoles");
    QTest::newRow("basicmodel") << "MODEL test(display)" << "test" << QList<ASTModelRole>({{"display"}});
    QTest::newRow("basicmodelsemicolon") << "MODEL test(display);" << "test" << QList<ASTModelRole>({{"display"}});
}

void tst_Parser::testModels()
{
    QFETCH(QString, modelDeclaration);
    QFETCH(QString, expectedModel);
    QFETCH(QList<ASTModelRole>, expectedRoles);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << " // comment 1" << Qt::endl;
    stream << " // comment 2" << Qt::endl;
    stream << "class TestClass" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << "    // comment 3" << Qt::endl;
    stream << "    // comment 4" << Qt::endl;
    stream << modelDeclaration << " // comment 5" << Qt::endl;
    stream << "    // comment 6" << Qt::endl;
    stream << "    // comment 7" << Qt::endl;
    stream << "};" << Qt::endl;
    stream << " // comment 8" << Qt::endl;
    stream << " // comment 9" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 1);

    const ASTClass astClass = ast.classes.first();
    ASTModel model = astClass.modelMetadata.first();
    ASTProperty property = astClass.properties.at(model.propertyIndex);
    QCOMPARE(property.name, expectedModel);
    int i = 0;
    for (auto role : model.roles) {
        QCOMPARE(role.name, expectedRoles.at(i).name);
        i++;
    }
}

void tst_Parser::testClasses_data()
{
    QTest::addColumn<QString>("classDeclaration");
    QTest::addColumn<QString>("expectedType");
    QTest::addColumn<QString>("expectedName");
    QTest::newRow("basicclass") << "CLASS sub(subObject)" << "subObject" << "sub";
}

void tst_Parser::testClasses()
{
    QFETCH(QString, classDeclaration);
    QFETCH(QString, expectedType);
    QFETCH(QString, expectedName);

    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << " // comment 1" << Qt::endl;
    stream << " // comment 2" << Qt::endl;
    stream << "class subObject" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << "    // comment 3" << Qt::endl;
    stream << "    // comment 4" << Qt::endl;
    stream << "    PROP(int value) // comment 5" << Qt::endl;
    stream << "    // comment 6" << Qt::endl;
    stream << "    // comment 7" << Qt::endl;
    stream << "};" << Qt::endl;
    stream << " // comment 8" << Qt::endl;
    stream << " // comment 9" << Qt::endl;
    stream << "class parentObject" << Qt::endl;
    stream << "{" << Qt::endl;
    stream << "    // comment 10" << Qt::endl;
    stream << "    // comment 11" << Qt::endl;
    stream << classDeclaration << " // comment 12" << Qt::endl;
    stream << "    // comment 13" << Qt::endl;
    stream << "    // comment 14" << Qt::endl;
    stream << "};" << Qt::endl;
    stream << " // comment 15" << Qt::endl;
    stream << " // comment 16" << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(parser.parse());

    const AST ast = parser.ast();
    QCOMPARE(ast.classes.size(), 2);

    const ASTClass astSub = ast.classes.value(0);
    const ASTClass astObj = ast.classes.value(1);
    QVERIFY(astSub.compilerAttribute.isEmpty());
    QVERIFY(astObj.compilerAttribute.isEmpty());
    const ASTProperty property = astObj.properties.at(astObj.subClassPropertyIndices.at(0));
    QCOMPARE(property.name, expectedName);
    QCOMPARE(property.type, expectedType);
}

void tst_Parser::testInvalid_data()
{
    QTest::addColumn<QString>("content");
    QTest::addColumn<QString>("warning");

    QTest::newRow("pod_invalid") << "POD (int foo)" << ".?Unknown token encountered";
    QTest::newRow("pod_unbalancedparens") << "POD foo(int foo" << ".?Unknown token encountered";
    QTest::newRow("pod_inclass") << "class Foo\n{\nPOD foo(int)\n}" << ".?POD: Can only be used in global scope";
    QTest::newRow("class_noidentifier") << "class\n{\n}" << ".?Unknown token encountered";
    QTest::newRow("class_nested") << "class Foo\n{\nclass Bar\n}" << ".?class: Cannot be nested";
    QTest::newRow("prop_outsideclass") << "PROP(int foo)" << ".?PROP: Can only be used in class scope";
    QTest::newRow("prop_toomanyargs") << "class Foo\n{\nPROP(int int foo)\n}" << ".?Invalid property declaration: flag foo is unknown";
    QTest::newRow("prop_toomanymodifiers") << "class Foo\n{\nPROP(int foo READWRITE, READONLY)\n}" << ".?Invalid property declaration: combination not allowed .READWRITE, READONLY.";
    QTest::newRow("prop_noargs") << "class Foo\n{\nPROP()\n}" << ".?Unknown token encountered";
    QTest::newRow("prop_unbalancedparens") << "class Foo\n{\nPROP(int foo\n}" << ".?Unknown token encountered";
    QTest::newRow("signal_outsideclass") << "SIGNAL(foo())" << ".?SIGNAL: Can only be used in class scope";
    QTest::newRow("signal_noargs") << "class Foo\n{\nSIGNAL()\n}" << ".?Unknown token encountered";
    QTest::newRow("slot_outsideclass") << "SLOT(void foo())" << ".?SLOT: Can only be used in class scope";
    QTest::newRow("slot_noargs") << "class Foo\n{\nSLOT()\n}" << ".?Unknown token encountered";
    QTest::newRow("model_outsideclass") << "MODEL foo" << ".?Unknown token encountered";
    QTest::newRow("class_outsideclass") << "CLASS foo" << ".?Unknown token encountered";
    QTest::newRow("preprecessor_line_inclass") << "class Foo\n{\n#define foo\n}" << ".?Unknown token encountered";
}

void tst_Parser::testInvalid()
{
    QFETCH(QString, content);
    QFETCH(QString, warning);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning));
    QTemporaryFile file;
    file.open();
    QTextStream stream(&file);
    stream << content << Qt::endl;
    file.seek(0);

    RepParser parser(file);
    QVERIFY(!parser.parse());
}

QTEST_APPLESS_MAIN(tst_Parser)

#include "tst_parser.moc"
