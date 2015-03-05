TEMPLATE = subdirs
CONFIG += debug_and_release ordered
SUBDIRS = \
    server \
    CppClient \
    ModelViewClient \
    ModelViewServer

qtHaveModule(quick) {
    SUBDIRS += \
        plugins \
        ClientApp
}
