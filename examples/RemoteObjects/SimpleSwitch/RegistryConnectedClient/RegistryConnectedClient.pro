#-------------------------------------------------
#
# Project created by QtCreator 2015-02-13T15:22:50
#
#-------------------------------------------------

QT       += remoteobjects core

QT       -= gui

TARGET = RegistryConnectedClient
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

target.path = $$[QT_INSTALL_EXAMPLES]/RemoteObjects/SimpleSwitch/RegistryConnectedClient
INSTALLS += target

SOURCES += main.cpp \
    dynamicclient.cpp

HEADERS += \
    dynamicclient.h
