option(host_build)

load(qt_tool)
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_URL_CAST_FROM_STRING

SOURCES += main.cpp RepParser.cpp RepCodeGenerator.cpp
HEADERS += RepParser.h RepCodeGenerator.h
