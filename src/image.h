#ifndef IMAGE
#define IMAGE

#include <QSharedPointer>
#include <QWeakPointer>
#include <QList>
#include <QImage>

class Image;
typedef QList<QWeakPointer<Image> > ImagesRef;
typedef QSharedPointer<Image> ImagePtr;
typedef QList<ImagePtr> Images;
Q_DECLARE_METATYPE(Images)

class Image
{
    mutable QByteArray md5;
public:
    QImage bm_orig;
    QImage bm_display;
    // This indicates a relative scale, where 1.0 means bitmap pixels match display pixels on
    // a low res 96 dpi display. On a high dpi screen it will look scaled up. Higher values
    // look better on most screens.
    // This is all relative to GetContentScalingFactor.
    double display_scale;

    inline Image(const QImage &other, double sc)
        : bm_orig(other), display_scale(sc) {}
    bool operator==(const Image &other) const;

    void bitmapScale(double sc);

    void displayScale(double sc);

    void resetScale(double sc);

    QImage &display();

    const QByteArray &getCRC() const;

    static QImage scaleImg(const QImage &src, double sc);
};

#endif // IMAGE
