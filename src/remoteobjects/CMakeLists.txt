# Generated from remoteobjects.pro.

#####################################################################
## RemoteObjects Module:
#####################################################################

qt_add_module(RemoteObjects
    QMAKE_MODULE_CONFIG remoteobjects_repc
    SOURCES
        qconnection_local_backend.cpp qconnection_local_backend_p.h
        qconnection_tcpip_backend.cpp qconnection_tcpip_backend_p.h
        qconnectionfactories.cpp qconnectionfactories_p.h
        qremoteobjectabstractitemmodeladapter.cpp qremoteobjectabstractitemmodeladapter_p.h
        qremoteobjectabstractitemmodelreplica.cpp qremoteobjectabstractitemmodelreplica.h qremoteobjectabstractitemmodelreplica_p.h
        qremoteobjectabstractitemmodeltypes.h
        qremoteobjectdynamicreplica.cpp qremoteobjectdynamicreplica.h
        qremoteobjectnode.cpp qremoteobjectnode.h qremoteobjectnode_p.h
        qremoteobjectpacket.cpp qremoteobjectpacket_p.h
        qremoteobjectpendingcall.cpp qremoteobjectpendingcall.h qremoteobjectpendingcall_p.h
        qremoteobjectregistry.cpp qremoteobjectregistry.h
        qremoteobjectregistrysource.cpp qremoteobjectregistrysource_p.h
        qremoteobjectreplica.cpp qremoteobjectreplica.h qremoteobjectreplica_p.h
        qremoteobjectsettingsstore.cpp qremoteobjectsettingsstore.h
        qremoteobjectsource.cpp qremoteobjectsource.h qremoteobjectsource_p.h
        qremoteobjectsourceio.cpp qremoteobjectsourceio_p.h
        qtremoteobjectglobal.cpp qtremoteobjectglobal.h
    DEFINES
        QT_BUILD_REMOTEOBJECTS_LIB
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_CAST_TO_ASCII
        QT_NO_URL_CAST_FROM_STRING
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Network
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

#### Keys ignored in scope 1:.:.:remoteobjects.pro:<TRUE>:
# MODULE = "remoteobjects"
# MODULE_CONFIG = "remoteobjects_repc"
# OTHER_FILES = "doc/qtremoteobjects.qdocconf" "doc/src/remoteobjects-cpp.qdoc" "doc/src/remoteobjects-index.qdoc" "doc/src/remoteobjects-overview.qdoc" "doc/src/remoteobjects-repc.qdoc"

## Scopes:
#####################################################################

qt_extend_target(RemoteObjects CONDITION QNX
    SOURCES
        qconnection_qnx_backend.cpp qconnection_qnx_backend_p.h
        qconnection_qnx_global_p.h
        qconnection_qnx_qiodevices.cpp qconnection_qnx_qiodevices.h qconnection_qnx_qiodevices_p.h
        qconnection_qnx_server.cpp qconnection_qnx_server.h qconnection_qnx_server_p.h
)

qt_extend_target(RemoteObjects CONDITION (QNX) AND (DEFINES ___contains___USE_HAM)
    PUBLIC_LIBRARIES
        ham
)
qt_add_docs(RemoteObjects
    doc/qtremoteobjects.qdocconf
)

include(Qt6RemoteObjectsMacros.cmake)    # special case