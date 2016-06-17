QT       += remoteobjects core

QT       -= gui

TARGET = DirectConnectClient
CONFIG   += console
CONFIG   -= app_bundle

REPC_REPLICA = simpleswitch.rep

TEMPLATE = app

target.path = $$[QT_INSTALL_EXAMPLES]/RemoteObjects/SimpleSwitch/DirectConnectClient
INSTALLS += target

SOURCES += main.cpp \
    client.cpp

HEADERS += \
    client.h

DISTFILES += \
    simpleswitch.rep
