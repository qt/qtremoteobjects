QT += remoteobjects

TARGET = QtSource
CONFIG += cmdline
CONFIG   -= app_bundle

REPC_SOURCE += Simple.rep

DISTFILES += \
    Simple.rep

SOURCES += main.cpp \
           simple.cpp

HEADERS += simple.h

OTHER_FILES += $$REPC_SOURCE

repfiles.files = $$REPC_SOURCE
repfiles.path += $$[QT_INSTALL_EXAMPLES]/remoteobjects/cbor_python/qt_source
pythonfiles.files = py_replica/py_replica.py
pythonfiles.path += $$[QT_INSTALL_EXAMPLES]/remoteobjects/cbor_python/py_replica

target.path = $$[QT_INSTALL_EXAMPLES]/remoteobjects/cbor_python/qt_source

INSTALLS += target repfiles pythonfiles
