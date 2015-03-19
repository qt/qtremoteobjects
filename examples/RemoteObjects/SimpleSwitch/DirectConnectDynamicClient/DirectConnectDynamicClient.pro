QT       += remoteobjects core

QT       -= gui

TARGET = DirectConnectDynamicClient
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    dynamicclient.cpp

HEADERS += \
    dynamicclient.h
