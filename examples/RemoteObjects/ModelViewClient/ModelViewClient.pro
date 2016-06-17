SOURCES = main.cpp

CONFIG   -= app_bundle

# install
target.path = $$[QT_INSTALL_EXAMPLES]/RemoteObjects/ModelViewClient
INSTALLS += target

QT += widgets remoteobjects
