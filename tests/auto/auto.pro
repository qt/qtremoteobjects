TEMPLATE = subdirs

sub_localsockettestserver.subdir = localsockettestserver
sub_localsockettestserver.target = sub-localsockettestserver

sub_integration.subdir = integration
sub_integration.target = sub-integration
sub_integration.depends = sub-localsockettestserver

SUBDIRS += benchmarks repc sub_integration modelview cmake pods repcodegenerator repparser \
           sub_localsockettestserver \
           modelreplica

qtConfig(process): SUBDIRS += integration_multiprocess
