TEMPLATE = subdirs
CONFIG += debug_and_release ordered
SUBDIRS = \
    server \
    cppclient \
    modelviewclient \
    modelviewserver \
    simpleswitch

contains(QT_CONFIG, ssl): SUBDIRS += ssl

qtHaveModule(quick) {
    SUBDIRS += \
        plugins \
        clientapp

    unix:!android: SUBDIRS += qmlmodelviewclient
}
