QT += qml remoteobjects

TEMPLATE = lib
CONFIG += plugin

REPC_REPLICA += $$PWD/../TimeModel.rep

DESTDIR = imports/TimeExample
TARGET  = qmlqtimeexampleplugin

SOURCES += plugin.cpp

pluginfiles.files += \
    imports/TimeExample/qmldir \
    imports/TimeExample/center.png \
    imports/TimeExample/clock.png \
    imports/TimeExample/Clock.qml \
    imports/TimeExample/hour.png \
    imports/TimeExample/minute.png

#qml.files = plugins.qml
#qml.path += $$[QT_HOST_PREFIX]/qml/plugins
target.path += $$[QT_HOST_PREFIX]/qml/TimeExample
pluginfiles.path += $$[QT_HOST_PREFIX]/qml/TimeExample

INSTALLS += target pluginfiles

contains(QT_CONFIG, c++11): CONFIG += c++11
