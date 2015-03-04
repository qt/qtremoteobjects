CONFIG += testcase parallel_test
TARGET = tst_integration
QT += testlib remoteobjects
QT -= gui

OTHER_FILES = engine.rep \
              speedometer.rep \
              ../repfiles/localdatacenter.rep \
              ../repfiles/tcpdatacenter.rep

REPC_SOURCE += $$OTHER_FILES
REPC_REPLICA += $$OTHER_FILES

HEADERS += $$PWD/engine.h \
           $$PWD/speedometer.h \
           $$PWD/temperature.h

SOURCES += $$PWD/engine.cpp \
           $$PWD/speedometer.cpp \
           $$PWD/tst_integration.cpp

contains(QT_CONFIG, c++11): CONFIG += c++11

