option(host_build)
!osx: CONFIG += force_bootstrap
include(3rdparty/moc/moc.pri)
QT = core-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_URL_CAST_FROM_STRING
DEFINES += RO_INSTALL_HEADERS=\\\"$$clean_path($$[QT_INSTALL_HEADERS])/QtRemoteObjects\\\"

SOURCES += \
    main.cpp \
    repparser.cpp \
    repcodegenerator.cpp \
    cppcodegenerator.cpp \
    utils.cpp

HEADERS += \
    repparser.h \
    repcodegenerator.h \
    cppcodegenerator.h \
    utils.h

load(qt_tool)
