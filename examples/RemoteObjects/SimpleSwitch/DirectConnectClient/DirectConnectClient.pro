QT       += remoteobjects core

QT       -= gui

TARGET = DirectConnectClient
CONFIG   += console
CONFIG   -= app_bundle

REPC_REPLICA = simpleswitch.rep

TEMPLATE = app


SOURCES += main.cpp \
    client.cpp

HEADERS += \
    client.h

DISTFILES += \
    simpleswitch.rep
