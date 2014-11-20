TARGET = QtRemoteObjects
MODULE = remoteobjects
MODULE_CONFIG = remoteobjects_repc
QT += network core-private
QT -= gui

QMAKE_DOCS = $$PWD/doc/qtremoteobjects.qdocconf

load(qt_module)

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_URL_CAST_FROM_STRING

INCLUDEPATH += $$PWD

PUBLIC_HEADERS += \
    $$PWD/qremoteobjectdynamicreplica.h \
    $$PWD/qremoteobjectsource.h \
    $$PWD/qremoteobjectreplica.h \
    $$PWD/qremoteobjectnode.h \
    $$PWD/qremoteobjectpendingcall.h \
    $$PWD/qtremoteobjectglobal.h \
    $$PWD/qremoteobjectregistry.h

PRIVATE_HEADERS += \
    $$PWD/qconnectionabstractfactory_p.h \
    $$PWD/qconnectionabstractserver_p.h \
    $$PWD/qconnectionclientfactory_p.h \
    $$PWD/qconnectionserverfactory_p.h \
    $$PWD/qremoteobjectsourceio_p.h \
    $$PWD/qremoteobjectsource_p.h \
    $$PWD/qregistrysource_p.h \
    $$PWD/qremoteobjectnode_p.h \
    $$PWD/qremoteobjectpacket_p.h \
    $$PWD/qremoteobjectpendingcall_p.h \
    $$PWD/qremoteobjectreplica_p.h

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

SOURCES += \
    $$PWD/qconnectionabstractserver.cpp \
    $$PWD/qconnectionclientfactory.cpp \
    $$PWD/qconnectionserverfactory.cpp \
    $$PWD/qremoteobjectdynamicreplica.cpp \
    $$PWD/qremoteobjectsource.cpp \
    $$PWD/qremoteobjectsourceio.cpp \
    $$PWD/qremoteobjectregistry.cpp \
    $$PWD/qregistrysource.cpp \
    $$PWD/qremoteobjectreplica.cpp \
    $$PWD/qremoteobjectnode.cpp \
    $$PWD/qremoteobjectpacket.cpp \
    $$PWD/qremoteobjectpendingcall.cpp \
    $$PWD/qtremoteobjectglobal.cpp

DEFINES += QT_BUILD_REMOTEOBJECTS_LIB

contains(QT_CONFIG, c++11): CONFIG += c++11
