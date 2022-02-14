if(TARGET Qt::WebSockets AND TARGET Qt::Widgets)
    qt_internal_add_example(wsclient)
    qt_internal_add_example(wsserver)
endif()
