QT += network testlib

QT -= gui

OTHER_FILES = engine.rep \
              speedometer.rep \
              ../repfiles/localdatacenter.rep \
              ../repfiles/tcpdatacenter.rep

REPC_SOURCE += $$OTHER_FILES
REPC_REPLICA += $$OTHER_FILES
QT += remoteobjects

CONFIG += testcase

HEADERS += $$PWD/RemoteObjectTest.h \
           $$PWD/Engine.h \
           $$PWD/Speedometer.h

SOURCES += $$PWD/Engine.cpp \
           $$PWD/Speedometer.cpp

contains(QT_CONFIG, c++11): CONFIG += c++11

