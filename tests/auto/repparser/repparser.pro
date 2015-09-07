CONFIG += testcase parallel_test
TARGET = tst_parser
QT += testlib core-private
QT -= gui

CONFIG += qlalr
QLALRSOURCES += $$top_srcdir/src/repparser/parser.g
INCLUDEPATH += $$top_srcdir/src/repparser

SOURCES += $$PWD/tst_parser.cpp

contains(QT_CONFIG, c++11):CONFIG += c++11
