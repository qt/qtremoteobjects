QT += widgets remoteobjects websockets
requires(qtConfig(treeview))

CONFIG -= app_bundle

HEADERS += websocketiodevice.h

SOURCES += \
    main.cpp \
    websocketiodevice.cpp

RESOURCES += sslcert.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/remoteobjects/websockets/wsserver
INSTALLS += target
