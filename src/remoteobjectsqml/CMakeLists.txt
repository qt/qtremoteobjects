
#####################################################################
## qtremoteobjects Plugin:
#####################################################################

qt_internal_add_qml_module(RemoteObjectsQml
    URI "QtRemoteObjects"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET declarative_remoteobjects
    CLASS_NAME QtRemoteObjectsPlugin
    SOURCES
        qremoteobjectsqml_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::RemoteObjects
)
