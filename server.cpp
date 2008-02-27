#include <QDebug>
#include <QSslKey>
#include "server.h"

static const QByteArray certData("\
-----BEGIN CERTIFICATE-----\n\
MIICBDCCAa6gAwIBAgIJANimnIGwYbPQMA0GCSqGSIb3DQEBBQUAMDgxCzAJBgNV\n\
BAYTAkZJMQ0wCwYDVQQIEwRPdWx1MQ0wCwYDVQQHEwRPdWx1MQswCQYDVQQKEwJP\n\
VDAeFw0wODAyMjUxOTMzNTNaFw0xMTAyMjQxOTMzNTNaMDgxCzAJBgNVBAYTAkZJ\n\
MQ0wCwYDVQQIEwRPdWx1MQ0wCwYDVQQHEwRPdWx1MQswCQYDVQQKEwJPVDBcMA0G\n\
CSqGSIb3DQEBAQUAA0sAMEgCQQCzVu78C26fVy+w92EAuNqFkWnbUm+g5kft2/NP\n\
CUKDlLAKyd+2MjMmb/bdBrk/7XiPvmp3DBSYkcRVV54RNRMPAgMBAAGjgZowgZcw\n\
HQYDVR0OBBYEFBtwZm20qbWlP33V1ktODkJ2aM+ZMGgGA1UdIwRhMF+AFBtwZm20\n\
qbWlP33V1ktODkJ2aM+ZoTykOjA4MQswCQYDVQQGEwJGSTENMAsGA1UECBMET3Vs\n\
dTENMAsGA1UEBxMET3VsdTELMAkGA1UEChMCT1SCCQDYppyBsGGz0DAMBgNVHRME\n\
BTADAQH/MA0GCSqGSIb3DQEBBQUAA0EAqjbS0xbPhuha86oG9pm70QE56oeqLg7N\n\
DZmVdCeFNjOZ40SpcdHgHPWwi6VjxjhsbM437Hrhj5OO5Tv7NseYNw==\n\
-----END CERTIFICATE-----\n");

static const QByteArray keyData("\
-----BEGIN RSA PRIVATE KEY-----\n\
MIIBOgIBAAJBALNW7vwLbp9XL7D3YQC42oWRadtSb6DmR+3b808JQoOUsArJ37Yy\n\
MyZv9t0GuT/teI++ancMFJiRxFVXnhE1Ew8CAwEAAQJAHh4VWGKbG0tLCQ8iTTq+\n\
B/xZqKl5ca36mml7vOQzNWF1DVjv1kXAydWRdv+baY0azYN5vssbn+pztYJG4SuX\n\
8QIhANleP962Tql1zJINxrz5y8AyNL6uBiiSOhteXtDq330DAiEA0zZ4q78kdIhv\n\
xdZXs31WypY9QXsJH0QeisYsMm/TNgUCIEtyDiMkDdt9mpqbE37mCNVV177TMioE\n\
zwxOzgQfcaC3AiEAhnfbg5yBnR9v6ch4tXFK3FeiUBoAyMMZnshkQwgms/ECIG9T\n\
+pMRGZmnFYt6k0TuW/CkU3O/zarvPg5E8tocjpgQ\n\
-----END RSA PRIVATE KEY-----\n");

static const QSslCertificate certificate(certData);
static const QSslKey key(keyData, QSsl::Rsa);

Server::Server(QObject* parent) : QTcpServer(parent)
{
    if (key.isNull())
        qDebug() << "SSL key is null";
    if (certificate.isNull())
        qDebug() << "SSL cert is null";
}

Server::~Server()
{
    foreach(QSslSocket* s, m_pendingConnections) {
        s->deleteLater();
    }
    m_pendingConnections.clear();
}

bool Server::hasPendingConnections() const
{
    return m_pendingConnections.count() != 0;
}

QTcpSocket* Server::nextPendingConnection()
{
    QTcpSocket*s = m_pendingConnections.takeFirst();
    s->disconnect();
    return s;
}

void Server::incomingConnection(int socketDescriptor)
{
    qDebug() << "Incoming connection";
    QSslSocket* socket = new QSslSocket;
    connect(socket, SIGNAL(encrypted()),
            this, SLOT(socketEncrypted()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(sslError(QList<QSslError>)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(stateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(modeChanged(QSslSocket::SslMode)),
            this, SLOT(modeChanged(QSslSocket::SslMode)));
    socket->setPrivateKey(key);
    socket->setLocalCertificate(certificate);
    if (socket->setSocketDescriptor(socketDescriptor)) {
        socket->startServerEncryption();
        qDebug() << "Started server encryption";
    } else {
        qDebug() << "Could not set socket descriptor!";
        socket->deleteLater();
    }
}

void Server::modeChanged(QSslSocket::SslMode mode)
{
    qDebug() << mode;
}

void Server::stateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << socketState;
}

void Server::sslError(const QList<QSslError>& errors)
{
    qDebug() << "SSL ERROR";
    foreach(QSslError e, errors) {
        qDebug() << e.error() << e.errorString();
        if (e.error() == QSslError::NoPeerCertificate) {
            qobject_cast<QSslSocket*>(sender())->ignoreSslErrors();
        }
    }
}

void Server::socketError(QAbstractSocket::SocketError e)
{
    qDebug() << e;
}

void Server::socketEncrypted()
{
    qDebug() << "socket encrypted";
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (socket) {
        qDebug() << "Socket go!";
        m_pendingConnections << socket;
        emit newConnection();
    }
}
