#include <QTcpServer>
#include <QDebug>
#include <QStringList>
#include <QtGlobal>
#include "manager.h"
#include "connectionwrapper.h"

Manager::Manager(QTcpServer* server, QObject* parent)
        :QObject(parent), m_server(server)
{
    connect(m_server, SIGNAL(newConnection()),
            this, SLOT(newConnection()));
}

void Manager::newConnection()
{
    if (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        ConnectionWrapper* w = new ConnectionWrapper(socket, this);
        connect(w, SIGNAL(gotMessage(QString)),
                this, SLOT(processMessage(QString)));
        connect(this, SIGNAL(message(QString)),
                w, SLOT(sendMessage(QString)));
    }
}

void Manager::processMessage(const QString& msg)
{
    ConnectionWrapper* c = qobject_cast<ConnectionWrapper*>(sender());
    Q_ASSERT(c);

    QString m = msg.trimmed();
    QString type;
    QStringList tokens;
    
    qDebug() << m;
    
    type = parseMessage(m, &tokens);
    if (type.isNull()) {
        qDebug() << "Received invalid command!";
        return;
    }

    // For unregistered connections only NICK command is supported...
    if (type == "NICK") {
        QString nick = tokens[0];
        if (c->isRegistered()) {
            emit message(QString("NICK %1 %2").arg(nick).arg(c->nick()));
        } else if (!c->isRegistered()){
            c->sendMessage(QString("NICK %1").arg(nick));
            m_connections[nick] = c;
            c->setRegistered(true);
            emit message(QString("STATUS %1 is now connected.").arg(nick));
        }
        c->setNick(nick);
    }

    // ... but registered connections get the whole shebang, so bail now
    // if the connection has not yet registered.
    if (!c->isRegistered())
        return;

    if (type == "SAY") {
        QString toks = tokens.join(" ");
        emit message(QString("SAY %1").arg(toks));
    }
}

QString Manager::parseMessage(const QString& m, QStringList* tokens)
{
    tokens->clear();

    QStringList words = m.split(" ");
    if (words.count() == 0)
        return QString();

    QString command = words.takeAt(0).toUpper();
    if (command == "NICK") { // NICK knows 2 arguments so take first 2
        for (int i = 0; i < 2 && words.count() != 0; i++) {
            tokens->append(words.takeAt(0));
        }
        return command;
    } else if (command == "SAY") {
        if (words.count() >= 2) { // SAY has 2 real arguments but n words
            tokens->append(words.takeAt(0));
            tokens->append(words.join(" "));
            return command;
        }
    }

    return QString();
}
