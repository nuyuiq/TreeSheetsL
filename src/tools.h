#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QIODevice>
#include <QColor>
#include <QPainter>

class QPainter;

enum DIO_BO{ DIO_BO_BIG, DIO_BO_Little };

namespace Tools {

//! 解析路径到真实路径
QString resolvePath(const QString &path, bool exist);

QString bakName(const QString &filename);
QString tmpName(const QString &filename);
QString extName(const QString &filename, const QString &ext);

int countCol(const QString &s);

void drawRect(QPainter &dc, int x, int y, int xs, int ys);
void drawRect(QPainter &dc, uint color, int x, int y, int xs, int ys, bool outline = false);
void drawRoundedRect(QPainter &dc, uint color, int roundness, int x, int y, int xs, int ys);
void drawRoundedRect(QPainter &dc, int roundness, const QRect &rect);
void drawLine(QPainter &dc, int x0, int y0, int x1, int y1);
void drawImage(QPainter &dc, int x, int y, const QImage &img);
void drawText(QPainter &dc, const QString &s, int x, int y, int w, int h);


class DataIO
{
    QIODevice*dev;
    DIO_BO bo;
public:
    DataIO(QIODevice*dev) : dev(dev), bo(DIO_BO_Little) {}
    inline void setByteOrder(DIO_BO bo) { this->bo = bo; }
    inline QIODevice *d() const { return dev; }
    inline qint64 pos()
    {
        return dev->pos();
    }
    inline bool seek(qint64 pos)
    {
        return dev->seek(pos);
    }

    QString readString();
    qint8 readInt8();
    qint32 readInt32();
    qint64 readInt64();
    double readDouble();
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

    void writeDouble(double num);
    inline qint64 write(const char *data, qint64 len)
    {
        return dev->write(data, len);
    }
    inline void writeInt8(char val) { dev->write(&val, 1); }
    void writeInt32(qint32 val);
    void writeInt64(qint64 val);
    void writeString(const QString &str);

};

class Painter : public QPainter
{
#ifdef QT_DEBUG
    Painter*_this;
#endif
    QPixmap*dp;
    QWidget*widget;
    QRect rect;
    int needUpdate;

public:
    Painter(const QWidget*widget);
    ~Painter();
    void update(const QRect &rect=QRect());

    inline static Painter *p(QPainter &dc)
    {
        Painter* _p = static_cast<Painter*>(&dc);
#ifdef QT_DEBUG
    Q_ASSERT(_p == _p->_this);
#endif
        return _p;
    }
};

}

//! 由于原系统项目环境使用的颜色编码为 BGR ，需要转换为 RGB
class Color : public QColor
{
public:
    inline Color(const QColor &other) : QColor(other) { }
    inline Color(uint bgr): QColor(uchar(bgr), uchar(bgr>>8), uchar(bgr>>16)) { }
    inline uint toBGR() const { return blue() * 256 * 256 + green() * 256 + red(); }
};

#define DELPTR(p) ({if (p) {delete p; p = nullptr;}})




namespace Dialog {
QString saveFile(const QString &title, const QString &filter, const QString &def=QString());
QString openFile(const QString &title, const QString & filter, const QString &def=QString());
int intValue(const QString &title, const QString &label, int min, int max, int def, int errret=-1);

}


#endif // TOOLS_H
