TEMPLATE = aux

prf.files = remoteobjects_repc.prf repcclient.pri repcserver.pri repcmerged.pri repccommon.pri repparser.prf
prf.path = $$[QT_HOST_DATA]/mkspecs/features
INSTALLS += prf

# Ensure files are copied to qtbase mkspecs for non-prefixed builds
!force_independent:if(!debug_and_release|!build_all|CONFIG(release, debug|release)) {
    prffiles2build.input = prf.files
    prffiles2build.output = $$[QT_HOST_DATA]/mkspecs/features/${QMAKE_FILE_IN}
    prffiles2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
    prffiles2build.name = COPY ${QMAKE_FILE_IN}
    prffiles2build.CONFIG = no_link target_predeps
    QMAKE_EXTRA_COMPILERS += prffiles2build
}
