CONFIG += testcase parallel_test
TARGET = tst_server2client
QT += testlib remoteobjects
QT -= gui

SOURCES += tst_server2client.cpp

REP_FILES = pods.h

QOBJECT_REP = $$REP_FILES

REPC_REPLICA = $$REP_FILES

contains(QT_CONFIG, c++11): CONFIG += c++11
