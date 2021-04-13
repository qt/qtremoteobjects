# Generated from remoteobjects.pro.

#####################################################################
## qtremoteobjects Plugin:
#####################################################################

qt_internal_add_qml_module(declarative_remoteobjects # special case
    URI "QtRemoteObjects"
    VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}" # special case
    CLASSNAME QtRemoteObjectsPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qremoteobjectsqml_p.h
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
        Qt::RemoteObjects
)

#### Keys ignored in scope 1:.:.:remoteobjects.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "5.$$QT_MINOR_VERSION"
# TARGETPATH = "QtRemoteObjects"
