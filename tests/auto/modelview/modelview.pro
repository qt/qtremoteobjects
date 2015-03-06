CONFIG += testcase parallel_test
TARGET = tst_modelview
QT += testlib remoteobjects
#QT -= gui

SOURCES += $$PWD/tst_modelview.cpp

contains(QT_CONFIG, c++11): CONFIG += c++11

