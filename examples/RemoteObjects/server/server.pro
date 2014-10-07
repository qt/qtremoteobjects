CONFIG   += console


REPC_SOURCE += ../TimeModel.rep
QT = remoteobjects core

SOURCES += TimeModel.cpp main.cpp
HEADERS += TimeModel.h

contains(QT_CONFIG, c++11): CONFIG += c++11

target.path = $$[QT_INSTALL_EXAMPLES]/RemoteObjects/Server
INSTALLS += target
