DEFINES += BACKEND=\\\"local\\\"
DEFINES += HOST_URL=\\\"local:replica\\\"
DEFINES += REGISTRY_URL=\\\"local:registry\\\"
CONFIG += testcase
TARGET = tst_integration_local
QT += testlib remoteobjects
QT -= gui

include(../template.pri)
