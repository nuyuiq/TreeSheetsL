#ifndef SINGLEINSTANCECHECKER_H
#define SINGLEINSTANCECHECKER_H

#include <QUdpSocket>

class Server : public QUdpSocket
{
    Q_OBJECT
public:
    Server(bool share, bool *isok = nullptr);

    void sendMsgToAnother(const QString &msg);

private slots:
    void readMsg();

private:
    void procMsg(const QString &msg);
};

#endif // SINGLEINSTANCECHECKER_H
