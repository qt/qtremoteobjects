CONFIG += testcase parallel_test
TARGET = tst_parser
QT += testlib
QT -= gui

INCLUDEPATH += $$PWD/../../../../tools/repc/
SOURCES += $$PWD/tst_parser.cpp $$PWD/../../../../tools/repc/RepParser.cpp

contains(QT_CONFIG, c++11):CONFIG += c++11
