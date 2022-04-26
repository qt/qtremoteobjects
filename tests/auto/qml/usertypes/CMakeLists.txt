
#####################################################################
## tst_usertypes Test:
#####################################################################
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_usertypes
    SOURCES
        tst_usertypes.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::RemoteObjects
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)
qt6_add_repc_merged(tst_usertypes
    usertypes.rep
)

qt_internal_extend_target(tst_usertypes CONDITION ANDROID OR IOS
        DEFINES
    QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_usertypes CONDITION NOT ANDROID AND NOT IOS
        DEFINES
    QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)

if(QT_BUILD_STANDALONE_TESTS)
    # For fully static builds, let qmlimportscanner take care of plugin importing
    qt_import_qml_plugins(tst_usertypes)
endif()

#### Keys ignored in scope 1:.:.:usertypes.pro:<TRUE>:
# TEMPLATE = "app"
