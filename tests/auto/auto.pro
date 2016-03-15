TEMPLATE = subdirs


sub_localsockettestserver.subdir = localsockettestserver
sub_localsockettestserver.target = sub-localsockettestserver

sub_integration.subdir = integration
sub_integration.target = sub-integration
sub_integration.depends = sub-localsockettestserver

SUBDIRS += repc sub_integration modelview cmake pods repcodegenerator repparser \
           sub_localsockettestserver

# TODO: benchmarks test flaky & very slow under Windows
SUBDIRS += benchmarks
