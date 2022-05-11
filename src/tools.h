#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QIODevice>

enum DIO_BO{ DIO_BO_BIG, DIO_BO_Little };

namespace Tools {

//! 解析路径到真实路径
QString resolvePath(const QString &path, bool exist);

QString bakName(const QString &filename);
QString tmpName(const QString &filename);
QString extName(const QString &filename, const QString &ext);

class DataIO
{
    QIODevice*dev;
    DIO_BO bo;
public:
    DataIO(QIODevice*dev) : dev(dev), bo(DIO_BO_Little) {}
    inline void setByteOrder(DIO_BO bo) { this->bo = bo; }

    QString readString();
    qint8 readInt8();
    qint32 readInt32();
    qint64 readInt64();
    double readDouble();


    inline QIODevice *d() const { return dev; }
    inline qint64 read(char *data, qint64 maxlen)
    {
        return dev->read(data, maxlen);
    }
    inline QByteArray read(qint64 maxlen)
    {
        return dev->read(maxlen);
    }
    inline QByteArray readAll()
    {
        return dev->readAll();
    }
    inline qint64 pos()
    {
        return dev->pos();
    }
    inline bool seek(qint64 pos)
    {
        return dev->seek(pos);
    }

};


}

#define DELPTR(p) ({if (p) {delete p; p = nullptr;}})

#endif // TOOLS_H
