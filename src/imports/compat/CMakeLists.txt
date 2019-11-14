# Generated from compat.pro.

#####################################################################
## qtqmlremoteobjects Plugin:
#####################################################################

qt_add_qml_module(qtqmlremoteobjects
    URI "QtQml.RemoteObjects"
    VERSION "1.0"
    CLASSNAME QtQmlRemoteObjectsPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::RemoteObjects
)

#### Keys ignored in scope 1:.:.:compat.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.0"
# TARGETPATH = "QtQml/RemoteObjects"
