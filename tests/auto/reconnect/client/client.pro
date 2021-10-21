TEMPLATE = app
TARGET = qtro_reconnect_client

DESTDIR = ./

QT += testlib remoteobjects
QT -= gui

CONFIG -= app_bundle

SOURCES += main.cpp
