CONFIG += testcase parallel_test
TARGET = tst_integration
QT += testlib remoteobjects
QT -= gui

#Only needed for tests, and only prior to
#commit a90bb5b89a09490a1795064133f6d8ce33b6874e
INCLUDEPATH += $$PWD

OTHER_FILES = engine.rep \
              ../repfiles/localdatacenter.rep \
              ../repfiles/tcpdatacenter.rep

REPC_SOURCE += $$OTHER_FILES
REPC_REPLICA += $$OTHER_FILES
REPC_MERGED += speedometer.rep

HEADERS += $$PWD/engine.h \
           $$PWD/speedometer.h \
           $$PWD/temperature.h

SOURCES += $$PWD/engine.cpp \
           $$PWD/speedometer.cpp \
           $$PWD/tst_integration.cpp

contains(QT_CONFIG, c++11): CONFIG += c++11

