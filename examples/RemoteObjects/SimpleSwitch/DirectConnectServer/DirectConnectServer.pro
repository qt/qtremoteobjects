QT       += remoteobjects core

QT       -= gui

TARGET = DirectConnectServer
CONFIG   += console
CONFIG   -= app_bundle

REPC_SOURCE = simpleswitch.rep

TEMPLATE = app

target.path = $$[QT_INSTALL_EXAMPLES]/RemoteObjects/SimpleSwitch/DirectConnectServer
INSTALLS += target

SOURCES += main.cpp \
    simpleswitch.cpp


HEADERS += \
    simpleswitch.h
