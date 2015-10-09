TEMPLATE = subdirs

repparser.path = $$[QT_INSTALL_HEADERS]/QtRepParser/
repparser.files += parser.g qrepregexparser.h
INSTALLS += repparser
