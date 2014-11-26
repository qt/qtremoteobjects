option(host_build)
CONFIG += force_bootstrap

load(qt_tool)
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_URL_CAST_FROM_STRING

SOURCES += main.cpp repparser.cpp repcodegenerator.cpp
HEADERS += repparser.h repcodegenerator.h
