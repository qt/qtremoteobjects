TEMPLATE = subdirs
CONFIG += debug_and_release ordered
SUBDIRS = \
    server \
    CppClient \
    ModelViewClient \
    ModelViewServer \
    SimpleSwitch

qtHaveModule(quick) {
    SUBDIRS += \
        plugins \
        ClientApp

    unix:!android: SUBDIRS += QMLModelViewClient
}
