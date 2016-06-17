QT       += remoteobjects core

QT       -= gui

TARGET = DirectConnectDynamicClient
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

target.path = $$[QT_INSTALL_EXAMPLES]/RemoteObjects/SimpleSwitch/DirectConnectDynamicClient
INSTALLS += target

SOURCES += main.cpp \
    dynamicclient.cpp

HEADERS += \
    dynamicclient.h
