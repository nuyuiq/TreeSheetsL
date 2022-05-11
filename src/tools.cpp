#include "tools.h"

#include <QApplication>
#include <QFileInfo>
#include <QtEndian>
#include <cmath>

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
