#ifndef PSAT_CONNECTIONWRAPPER_H
#define PSAT_CONNECTIONWRAPPER_H

#include <QObject>
#include <QAbstractSocket>

class QTcpSocket;

class ConnectionWrapper : public QObject
{
    Q_OBJECT
    public:
        ConnectionWrapper(QTcpSocket* socket, QObject* parent = 0);
        bool isRegistered() const
        { return m_registered; }
        void setRegistered(bool registered)
        { m_registered = registered; }
        QString nick() const
        { return m_nick; }
        void setNick(QString nick)
        { m_nick = nick; }

    public slots:
        void sendMessage(const QString& message);

    signals:
        void gotMessage(const QString& message);

    protected slots:
        void incoming();
        void stateChanged(QAbstractSocket::SocketState);

    private:
        QTcpSocket* m_socket;
        bool m_registered;
        QString m_nick;
};

#endif // PSAT_CONNECTIONWRAPPER_H
