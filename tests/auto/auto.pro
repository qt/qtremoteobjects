TEMPLATE = subdirs

sub_localsockettestserver.subdir = localsockettestserver
sub_localsockettestserver.target = sub-localsockettestserver

sub_integration.subdir = integration
sub_integration.target = sub-integration
sub_integration.depends = sub-localsockettestserver

SUBDIRS += \
    benchmarks \
    cmake \
    integration_multiprocess \
    modelreplica \
    modelview \
    pods \
    repc \
    repcodegenerator \
    repparser \
    sub_integration \
    sub_localsockettestserver \
