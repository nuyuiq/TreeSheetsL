#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QIODevice>
#include <QColor>

class QPainter;

enum DIO_BO{ DIO_BO_BIG, DIO_BO_Little };

namespace Tools {

//! 解析路径到真实路径
QString resolvePath(const QString &path, bool exist);

QString bakName(const QString &filename);
QString tmpName(const QString &filename);
QString extName(const QString &filename, const QString &ext);

void drawRect(QPainter &dc, uint color, int x, int y, int xs, int ys, bool outline = false);
void drawRoundedRect(QPainter &dc, uint color, int roundness, int x, int y, int xs, int ys);


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

//! 由于原系统项目环境使用的颜色编码为 BGR ，需要转换为 RGB
class Color : public QColor
{
public:
    inline Color(uint bgr): QColor(uchar(bgr), uchar(bgr>>8), uchar(bgr>>16)) { }
    inline uint toBGR() const { return blue() * 256 * 256 + green() * 256 + red(); }
};

#define DELPTR(p) ({if (p) {delete p; p = nullptr;}})

#endif // TOOLS_H
