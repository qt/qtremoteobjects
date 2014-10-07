TEMPLATE = subdirs
CONFIG += debug_and_release ordered
SUBDIRS = \
    server \
    CppClient

qtHaveModule(quick) {
    SUBDIRS += \
        plugins \
        ClientApp
}
