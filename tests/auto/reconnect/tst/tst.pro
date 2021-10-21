TEMAPLATE=app
TARGET = tst_reconnect

DESTDIR = ./

QT += testlib remoteobjects
QT -= gui

CONFIG += testcase
CONFIG -= app_bundle

SOURCES += tst_reconnect.cpp
