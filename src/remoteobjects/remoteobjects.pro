TARGET = QtRemoteObjects
MODULE = remoteobjects
MODULE_CONFIG = remoteobjects_repc
QT += network core-private
QT -= gui

QMAKE_DOCS = $$PWD/doc/qtremoteobjects.qdocconf
OTHER_FILES += \
    doc/qtremoteobjects.qdocconf \
    doc/src/remoteobjects-cpp.qdoc \
    doc/src/remoteobjects-index.qdoc \
    doc/src/remoteobjects-overview.qdoc \
    doc/src/remoteobjects-repc.qdoc

load(qt_module)

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_URL_CAST_FROM_STRING

INCLUDEPATH *= .

HEADERS += \
    qconnection_local_backend_p.h \
    qconnection_tcpip_backend_p.h \
    qconnectionfactories_p.h \
    qremoteobjectabstractitemmodeladapter_p.h \
    qremoteobjectabstractitemmodelreplica.h \
    qremoteobjectabstractitemmodelreplica_p.h \
    qremoteobjectabstractitemmodeltypes.h \
    qremoteobjectdynamicreplica.h \
    qremoteobjectnode.h \
    qremoteobjectnode_p.h \
    qremoteobjectpacket_p.h \
    qremoteobjectpendingcall.h \
    qremoteobjectpendingcall_p.h \
    qremoteobjectregistry.h \
    qremoteobjectregistrysource_p.h \
    qremoteobjectreplica.h \
    qremoteobjectreplica_p.h \
    qremoteobjectsettingsstore.h \
    qremoteobjectsource.h \
    qremoteobjectsource_p.h \
    qremoteobjectsourceio_p.h \
    qtremoteobjectglobal.h

SOURCES += \
    qconnection_local_backend.cpp \
    qconnection_tcpip_backend.cpp \
    qconnectionfactories.cpp \
    qremoteobjectabstractitemmodeladapter.cpp \
    qremoteobjectabstractitemmodelreplica.cpp \
    qremoteobjectdynamicreplica.cpp \
    qremoteobjectnode.cpp \
    qremoteobjectpacket.cpp \
    qremoteobjectpendingcall.cpp \
    qremoteobjectregistry.cpp \
    qremoteobjectregistrysource.cpp \
    qremoteobjectreplica.cpp \
    qremoteobjectsettingsstore.cpp \
    qremoteobjectsource.cpp \
    qremoteobjectsourceio.cpp \
    qtremoteobjectglobal.cpp

qnx {
    SOURCES += \
        qconnection_qnx_backend.cpp \
        qconnection_qnx_qiodevices.cpp \
        qconnection_qnx_server.cpp

    HEADERS += \
        qconnection_qnx_backend_p.h \
        qconnection_qnx_global_p.h \
        qconnection_qnx_qiodevices.h \
        qconnection_qnx_qiodevices_p.h \
        qconnection_qnx_server.h \
        qconnection_qnx_server_p.h

    contains(DEFINES , USE_HAM) {
      LIBS += -lham
      message(configured for using HAM)
    } else {
      message(configured without using HAM)
    }
}

DEFINES += QT_BUILD_REMOTEOBJECTS_LIB

contains(QT_CONFIG, c++11): CONFIG += c++11
