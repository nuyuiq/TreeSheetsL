#include "tools.h"

#include <QApplication>
#include <QFileInfo>
#include <QtEndian>
#include <cmath>
#include <QPainter>
#include <QDebug>
#include <QWidget>

QString Tools::resolvePath(const QString &path, bool exist)
{
    QString sp = path.isEmpty()? QStringLiteral("."): path;
    bool isrp =
#ifdef Q_OS_WIN
    sp.length() > 2 && sp.at(1) == QChar::fromLatin1(':');
#else
    sp.at(0) == "/";
#endif
    if (!isrp) sp = QApplication::applicationDirPath() + "/" + sp;
    if (exist && !QFileInfo::exists(sp)) sp.clear();
    return sp;
}

QString Tools::bakName(const QString &filename)
{
    return extName(filename, QStringLiteral(".bak"));
}

QString Tools::tmpName(const QString &filename)
{
    return extName(filename, QStringLiteral(".tmp"));
}

QString Tools::extName(const QString &filename, const QString &ext)
{
    return QFileInfo(filename).filePath() + ext;
}

QString Tools::DataIO::readString()
{
    int len = readInt32();
    if (len == 0) return QString();
    QByteArray d = read(len);
    if (d.size() != len) return QString();
    return QString::fromUtf8(d);
}

qint8 Tools::DataIO::readInt8()
{
    char buff[1];
    if (read(buff, 1) != 1) return 0;
    return buff[0];
}

qint32 Tools::DataIO::readInt32()
{
    char buff[4];
    if (read(buff, 4) != 4) return 0;
    if (Q_LIKELY(bo == DIO_BO_Little))
    {
        return qFromLittleEndian<qint32>((uchar*)buff);
    }
    return qFromBigEndian<qint32>((uchar*)buff);
}

qint64 Tools::DataIO::readInt64()
{
    char buff[8];
    if (read(buff, 8) != 8) return 0;
    if (Q_LIKELY(bo == DIO_BO_Little))
    {
        return qFromLittleEndian<qint64>((uchar*)buff);
    }
    return qFromBigEndian<qint64>((uchar*)buff);
}

double Tools::DataIO::readDouble()
{
    // USE_APPLE_IEEE
    // useExtendedPrecision
    char bytes[10];
    if (read(bytes, 10) != 10) return 0.;

    double f;
    qint32 expon;
    quint32 hiMant, loMant;

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant = ((quint32)(bytes[2] & 0xFF) << 24)
            | ((quint32)(bytes[3] & 0xFF) << 16)
            | ((quint32)(bytes[4] & 0xFF) << 8)
            | ((quint32)(bytes[5] & 0xFF));
    loMant = ((quint32)(bytes[6] & 0xFF) << 24)
            | ((quint32)(bytes[7] & 0xFF) << 16)
            | ((quint32)(bytes[8] & 0xFF) << 8)
            | ((quint32)(bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0)
    {
        f = 0;
    }
    else
    {
        if (expon == 0x7FFF)
        { /* Infinity or NaN */
            f = HUGE_VAL;
        }
        else
        {
            expon -= 16383;
            f  = ldexp(double (hiMant), expon-=31);
            f += ldexp(double (loMant), expon-=32);
        }
    }
    return (bytes[0] & 0x80)? -f: f;
}

void Tools::drawRect(QPainter &dc, uint color, int x, int y, int xs, int ys, bool outline)
{    
    if (!dc.hasClipping())
    {
        Painter::p(dc)->update(QRect(x, y, xs, ys));
        return;
    }
    const Color qcolor(color);
    if (outline) dc.setBrush(QBrush(Qt::transparent));
    else dc.setBrush(QBrush(qcolor));
    dc.setPen(QPen(qcolor));
    dc.drawRect(x, y, xs - 1, ys - 1);
}

void Tools::drawRoundedRect(QPainter &dc, uint color, int roundness, int x, int y, int xs, int ys)
{
    if (!dc.hasClipping())
    {
        Painter::p(dc)->update(QRect(x, y, xs, ys));
        return;
    }
    const Color qcolor(color);
    dc.setBrush(QBrush(qcolor));
    dc.setPen(QPen(qcolor));
    const QRect rect(x, y, xs - 1, ys - 1);
    dc.drawRoundedRect(rect, roundness, roundness, Qt::AbsoluteSize);
}

void Tools::drawRect(QPainter &dc, int x, int y, int xs, int ys)
{
    if (!dc.hasClipping())
    {
        Painter::p(dc)->update(QRect(x, y, xs, ys));
    }
    else
    {
        dc.drawRect(x, y, xs, ys);
    }
}

Tools::Painter::Painter(const QWidget *widget)
{
    Q_ASSERT(widget);
    this->widget = const_cast<QWidget*>(widget);
    needUpdate = 0;
    dp = new QPixmap(widget->size());
    begin(dp);
#ifdef QT_DEBUG
    _this = this;
#endif
}

Tools::Painter::~Painter()
{
    end();
    delete dp;
    if (needUpdate == 1)
    {
        widget->update(rect);
    }
    else if (needUpdate == 2)
    {
        widget->update();
    }
}

void Tools::Painter::update(const QRect &rect)
{
#ifdef QT_DEBUG
    Q_ASSERT(_this == this);
#endif
    if (needUpdate == 2) return;
    if (rect.isNull())
    {
        needUpdate = 2;
        return;
    }
    needUpdate = 1;
    this->rect |= transform().mapRect(rect);
}

void Tools::drawLine(QPainter &dc, int x0, int y0, int x1, int y1)
{
    if (!dc.hasClipping())
    {
        const QPoint tl(qMin(x0, x1) - 1, qMin(y0, y1) - 1);
        const QPoint br(qMax(x0, x1) + 1, qMax(y0, y1) + 1);
        Painter::p(dc)->update(QRect(tl, br));
    }
    else
    {
        dc.drawLine(x0, y0, x1, y1);
    }
}

void Tools::drawRoundedRect(QPainter &dc, int roundness, const QRect &rect)
{
    if (!dc.hasClipping())
    {
        Painter::p(dc)->update(rect);
    }
    else
    {
    dc.drawRoundedRect(rect, roundness, roundness, Qt::AbsoluteSize);
    }
}

void Tools::drawImage(QPainter &dc, int x, int y, const QImage &img)
{
    if (!dc.hasClipping())
    {
        Painter::p(dc)->update(QRect(QPoint(x, y), img.size()));
    }
    else
    {
        dc.drawImage(x, y, img);
    }
}

void Tools::drawText(QPainter &dc, const QString &s, int x, int y, int w, int h)
{
    QRect rect = dc.boundingRect(x, y, w, h, Qt::AlignLeft | Qt::AlignVCenter, s);
    if (!dc.hasClipping())
    {
        Painter::p(dc)->update(rect);
    }
    else
    {
        dc.drawText(rect, s);
    }
}
