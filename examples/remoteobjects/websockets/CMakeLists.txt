# Generated from websockets.pro.

if(TARGET Qt::WebSockets AND TARGET Qt::Widgets)
    add_subdirectory(wsclient)
    add_subdirectory(wsserver)
endif()
