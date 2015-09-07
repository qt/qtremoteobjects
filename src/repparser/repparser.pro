TARGET = QtRepParser
CONFIG += staticlib
QT = core-private

load(qt_module)

CONFIG += qlalr
QLALRSOURCES += $$top_srcdir/src/repparser/parser.g

OTHER_FILES += parser.g

HEADERS += qregexparser.h
