DEFINES += BACKEND=\\\"local\\\"
DEFINES += HOST_URL=\\\"local:replica_local_integration\\\"
DEFINES += REGISTRY_URL=\\\"local:registry_local_integration\\\"
TARGET = tst_integration_local

include(../template.pri)
