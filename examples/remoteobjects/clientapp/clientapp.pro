QT += remoteobjects quick

CONFIG += qmltypes

REPC_REPLICA += $$PWD/../timemodel.rep
TARGET  = clientapp

QML_IMPORT_NAME = TimeExample
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += main.cpp plugin.cpp
HEADERS += plugin.h

RESOURCES += clientapp.qrc

contains(QT_CONFIG, c++11): CONFIG += c++11

target.path = $$[QT_INSTALL_EXAMPLES]/remoteobjects/clientapp
INSTALLS += target
