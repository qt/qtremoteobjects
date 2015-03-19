#-------------------------------------------------
#
# Project created by QtCreator 2015-02-13T15:02:06
#
#-------------------------------------------------

QT       += remoteobjects core

QT       -= gui

TARGET = RegistryConnectedServer
CONFIG   += console
CONFIG   -= app_bundle

REPC_SOURCE = SimpleSwitch.rep


TEMPLATE = app


SOURCES += main.cpp \
    simpleswitch.cpp

HEADERS += \
    simpleswitch.h
