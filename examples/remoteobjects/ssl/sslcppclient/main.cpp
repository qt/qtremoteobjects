// Copyright (C) 2017 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCoreApplication>
#include <QHostAddress>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslKey>
#include <QTimer>
#include "rep_timemodel_replica.h"

#include <QRemoteObjectNode>

class tester : public QObject
{
    Q_OBJECT
public:
    tester() : QObject(nullptr)
    {
        QRemoteObjectNode m_client;
        auto socket = setupConnection();
        connect(socket, &QSslSocket::errorOccurred,
                socket, [](QAbstractSocket::SocketError error){
            qDebug() << "QSslSocket::error" << error;
        }) ;
        m_client.addClientSideConnection(socket);

        ptr1.reset(m_client.acquire< MinuteTimerReplica >());
        ptr2.reset(m_client.acquire< MinuteTimerReplica >());
        ptr3.reset(m_client.acquire< MinuteTimerReplica >());
        QTimer::singleShot(0, this, &tester::clear);
        QTimer::singleShot(1, this, &tester::clear);
        QTimer::singleShot(10000, this, &tester::clear);
        QTimer::singleShot(11000, this, &tester::clear);
    }
public slots:
    void clear()
    {
        static int i = 0;
        if (i == 0) {
            i++;
            ptr1.reset();
        } else if (i == 1) {
            i++;
            ptr2.reset();
        } else if (i == 2) {
            i++;
            ptr3.reset();
        } else {
            qApp->quit();
        }
    }

private:
    QScopedPointer<MinuteTimerReplica> ptr1, ptr2, ptr3;

    QSslSocket *setupConnection()
    {
        auto socketClient = new QSslSocket;
        socketClient->setLocalCertificate(QStringLiteral(":/sslcert/client.crt"));
        socketClient->setPrivateKey(QStringLiteral(":/sslcert/client.key"));
        socketClient->setPeerVerifyMode(QSslSocket::VerifyPeer);
        socketClient->connectToHostEncrypted(QStringLiteral("127.0.0.1"), 65511);
        if (!socketClient->waitForEncrypted(-1)) {
            qWarning("Failed to connect to server %s",
                   qPrintable(socketClient->errorString()));
            exit(0);
        }
        return socketClient;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    auto config = QSslConfiguration::defaultConfiguration();
    config.setCaCertificates(QSslCertificate::fromPath(QStringLiteral(":/sslcert/rootCA.pem")));
    QSslConfiguration::setDefaultConfiguration(config);

    tester t;
    return a.exec();
}

#include "main.moc"
