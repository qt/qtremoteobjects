QT       += remoteobjects core

QT       -= gui

TARGET = DirectConnectServer
CONFIG   += console
CONFIG   -= app_bundle

REPC_SOURCE = simpleswitch.rep

TEMPLATE = app


SOURCES += main.cpp \
    simpleswitch.cpp


HEADERS += \
    simpleswitch.h
