CONFIG += testcase
TARGET = tst_rep_from_header
QT += testlib remoteobjects
QT -= gui

SOURCES += tst_rep_from_header.cpp

REP_FILES = pods.h

QOBJECT_REP = $$REP_FILES
