TARGET = QtRepParser
MODULE = repparser

# Dont produce a library
CONFIG += header_module

load(qt_module)

# Fixup DLLDESTDIR from qt_module.pri
win32:!wince: DLLDESTDIR = $$MODULE_BASE_OUTDIR/bin

# Copy non-C++ headers to out dir as well (syncqt does not take care of it)
copy_files.name = copy_files
copy_files.input = EXTRA_HEADERS
copy_files.output = $${MODULE_BASE_OUTDIR}/include/$${MODULE_INCNAME}/${QMAKE_FILE_BASE}${QMAKE_FILE_EXT}
copy_files.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copy_files.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += copy_files

EXTRA_HEADERS += \
    $$PWD/parser.g

extra_headers.files = $$EXTRA_HEADERS
# Replicates a bit of logic of qtbase.git:mkspecs/features/qt_module_headers.prf
# Make sure we install parser.g to the resp. Frameworks include path if required
prefix_build:lib_bundle {
    extra_headers.path = $$[QT_INSTALL_LIBS]/QtRepParser.framework/Headers
} else {
    extra_headers.path = $$[QT_INSTALL_HEADERS]/$$MODULE_INCNAME
}
INSTALLS += extra_headers

PUBLIC_HEADERS += \
    $$PWD/qrepregexparser.h

HEADERS += $$PUBLIC_HEADERS
