CONFIG += testcase parallel_test
TARGET = tst_parser
QT += testlib core-private repparser
QT -= gui

SOURCES += $$PWD/tst_parser.cpp

contains(QT_CONFIG, c++11):CONFIG += c++11
