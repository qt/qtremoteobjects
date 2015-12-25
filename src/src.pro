TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    remoteobjects \
    repparser

qtHaveModule(quick) {
    SUBDIRS += imports
}
