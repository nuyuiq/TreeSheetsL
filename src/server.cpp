#include "server.h"
#include "symdef.h"

#include <QDebug>

Server::Server(bool share, bool *isok)
{
    // 不同平台，对 flag 的支持程度不同，某些配置空能无效
    auto f = share?
            QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint :
            QAbstractSocket::DontShareAddress;
    bool isbind = bind(QHostAddress::LocalHost, PORT_SERVER, f);
    if (isok) *isok = isbind;
    connect(this, SIGNAL(readyRead()), this, SLOT(readMsg()));
}

void Server::sendMsgToAnother(const QString &msg)
{
    // 即用即消
    writeDatagram(msg.toLocal8Bit(), QHostAddress(QHostAddress::LocalHost), PORT_SERVER);
}

void Server::readMsg()
{
    char buff[512];
    while (hasPendingDatagrams())
    {
        auto len = readDatagram(buff, sizeof (buff));
        if (len > 0) procMsg(QString::fromLocal8Bit(buff, len));
    }
}

void Server::procMsg(const QString &msg)
{
    // todo
    Q_UNUSED(msg)
    qDebug() << msg;
}

