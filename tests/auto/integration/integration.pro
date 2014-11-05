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

HEADERS += $$PWD/remoteobjecttest.h \
           $$PWD/engine.h \
           $$PWD/speedometer.h

SOURCES += $$PWD/engine.cpp \
           $$PWD/speedometer.cpp

contains(QT_CONFIG, c++11): CONFIG += c++11

