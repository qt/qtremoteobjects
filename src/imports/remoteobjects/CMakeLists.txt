# Generated from remoteobjects.pro.

#####################################################################
## qtremoteobjects Plugin:
#####################################################################

qt_add_qml_module(qtremoteobjects
    URI "QtRemoteObjects"
    VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}" # special case
    CLASSNAME QtRemoteObjectsPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::RemoteObjects
)

#### Keys ignored in scope 1:.:.:remoteobjects.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "5.$$QT_MINOR_VERSION"
# TARGETPATH = "QtRemoteObjects"
