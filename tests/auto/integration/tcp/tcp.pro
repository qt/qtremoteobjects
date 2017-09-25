DEFINES += BACKEND=\\\"tcp\\\"
DEFINES += HOST_URL=\\\"tcp://127.0.0.1:65511\\\"
DEFINES += REGISTRY_URL=\\\"tcp://127.0.0.1:65512\\\"
TARGET = tst_integration_tcp

include(../template.pri)
