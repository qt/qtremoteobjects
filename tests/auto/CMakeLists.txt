# Generated from auto.pro.

add_subdirectory(benchmarks)
# add_subdirectory(cmake) # special case
add_subdirectory(modelreplica)
add_subdirectory(modelview)
add_subdirectory(pods)
add_subdirectory(proxy)
add_subdirectory(repc)
add_subdirectory(repcodegenerator)
add_subdirectory(repparser)
add_subdirectory(subclassreplica)
if(QT_FEATURE_ssl) # special case
    add_subdirectory(external_IODevice)
endif()
if(TARGET Qt::Qml)
    add_subdirectory(qml)
endif()
if(QT_FEATURE_process)
    add_subdirectory(integration_multiprocess)
    add_subdirectory(proxy_multiprocess)
    add_subdirectory(integration_external)
    add_subdirectory(restart)
endif()

# special case begin
add_subdirectory(localsockettestserver)
add_subdirectory(integration)
# special case end
