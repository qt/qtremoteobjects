TARGET = QtRepParser
MODULE = repparser

# Dont produce a library
CONFIG += header_module

load(qt_module)

# Copy non-C++ headers to out dir as well (syncqt does not take care of it)
copy_files.input = EXTRA_HEADERS
copy_files.output = $${MODULE_BASE_OUTDIR}/include/$${MODULE_INCNAME}/${QMAKE_FILE_BASE}${QMAKE_FILE_EXT}
copy_files.commands = ${COPY_FILE} ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copy_files.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += copy_files

EXTRA_HEADERS += \
    $$PWD/parser.g

PUBLIC_HEADERS += \
    $$PWD/qrepregexparser.h

HEADERS += $$PUBLIC_HEADERS
