#include "image.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDebug>

bool Image::operator==(const Image &other) const
{
    return bm_orig.size() == other.bm_orig.size() &&
            getCRC() == other.getCRC();
}

void Image::bitmapScale(double sc)
{
    bm_orig = scaleImg(bm_orig, sc);
    bm_display = QImage();
}

void Image::displayScale(double sc)
{
    display_scale /= sc;
    bm_display = QImage();
}

void Image::resetScale(double sc)
{
    display_scale = sc;
    bm_display = QImage();
}

QImage &Image::display()
{
    if (bm_display.isNull())
    {
        bm_display = scaleImg(bm_orig, 1.0 / display_scale);
    }
    return bm_display;
}

const QByteArray &Image::getCRC() const
{
    if (md5.isEmpty() && !bm_orig.isNull())
    {
        QByteArray data;
        QBuffer buff(&data);
        buff.open(QIODevice::WriteOnly);
        bm_orig.save(&buff, "PNG");
        md5 = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    }
    return md5;
}

QImage Image::scaleImg(const QImage &src, double sc)
{
    const auto &s = src.size() * sc;
    return src.scaled(s, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
