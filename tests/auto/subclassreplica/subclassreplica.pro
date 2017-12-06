QT       += testlib remoteobjects

QT       -= gui

REPC_MERGED += class.rep

TARGET = tst_subclassreplicatest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_subclassreplicatest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
