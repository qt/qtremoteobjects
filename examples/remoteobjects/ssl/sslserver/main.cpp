// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "timemodel.h"

#include <QCoreApplication>
#include <QSslConfiguration>

#include "sslserver.h"

#include <QRemoteObjectHost>
/*
* http://stackoverflow.com/questions/7404163/windows-handling-ctrlc-in-different-thread
*/

void SigIntHandler()
{
    qDebug()<<"Ctrl-C received.  Quitting.";
    qApp->quit();
}

#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_QNX)
#include <signal.h>

void unix_handler(int s)
{
    if (s==SIGINT)
        SigIntHandler();
}

#elif defined(Q_OS_WIN32)
#include <windows.h>

BOOL WINAPI WinHandler(DWORD CEvent)
{
    switch (CEvent)
    {
    case CTRL_C_EVENT:
        SigIntHandler();
        break;
    }
    return TRUE;
}
#endif

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    auto config = QSslConfiguration::defaultConfiguration();
    config.setCaCertificates(QSslCertificate::fromPath(QStringLiteral(":/sslcert/rootCA.pem")));
    QSslConfiguration::setDefaultConfiguration(config);

#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_QNX)
    signal(SIGINT, &unix_handler);
#elif defined(Q_OS_WIN32)
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)WinHandler, TRUE);
#endif
    QRemoteObjectHost host;
    SslServer server;
    server.listen(QHostAddress::Any, 65511);

    host.setHostUrl(server.serverAddress().toString(), QRemoteObjectHost::AllowExternalRegistration);

    QObject::connect(&server, &SslServer::encryptedSocketReady, &server, [&host](QSslSocket *socket) {
        QObject::connect(socket, &QSslSocket::errorOccurred,
                socket, [](QAbstractSocket::SocketError error){
            qDebug() << "QSslSocket::error" << error;
        }) ;
        host.addHostSideConnection(socket);
    });

    MinuteTimer timer;
    host.enableRemoting(&timer);

    Q_UNUSED(timer)
    return app.exec();
}

