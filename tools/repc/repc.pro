option(host_build)
!osx: CONFIG += force_bootstrap
include(3rdparty/moc/moc.pri)

force_bootstrap:isEqual(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 5): error("compiling repc for bootstrap requires Qt 5.5 or higher, due to missing libraries.")

QT = core-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_URL_CAST_FROM_STRING
DEFINES += RO_INSTALL_HEADERS=\\\"$$clean_path($$[QT_INSTALL_HEADERS])/QtRemoteObjects\\\"

CONFIG += qlalr
QLALRSOURCES += $$top_srcdir/src/repparser/parser.g
INCLUDEPATH += $$top_srcdir/src/repparser

SOURCES += \
    main.cpp \
    repcodegenerator.cpp \
    cppcodegenerator.cpp \
    utils.cpp

HEADERS += \
    repcodegenerator.h \
    cppcodegenerator.h \
    utils.h

load(qt_tool)
