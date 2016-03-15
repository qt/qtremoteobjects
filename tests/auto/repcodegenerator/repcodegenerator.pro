CONFIG += testcase
TARGET = tst_repcodegenerator
QT += testlib remoteobjects
QT -= gui

SOURCES += tst_repcodegenerator.cpp

REP_FILES = preprocessortest.rep

REPC_REPLICA = $$REP_FILES

contains(QT_CONFIG, c++11): CONFIG += c++11
