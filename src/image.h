#ifndef IMAGE
#define IMAGE

#include <QSharedPointer>
#include <QWeakPointer>
#include <QList>
#include <QImage>

struct Image;
typedef QList<QWeakPointer<Image> > ImagesRef;
typedef QSharedPointer<Image> ImagePtr;
typedef QList<ImagePtr> Images;
Q_DECLARE_METATYPE(Images)

//typedef QImage Image;
struct Image {
    QImage bm_orig;
    QImage bm_display;
    // This indicates a relative scale, where 1.0 means bitmap pixels match display pixels on
    // a low res 96 dpi display. On a high dpi screen it will look scaled up. Higher values
    // look better on most screens.
    // This is all relative to GetContentScalingFactor.
    double display_scale;

    Image(const QImage &other, double sc)
        : bm_orig(other), display_scale(sc) {}
    inline bool operator==(const Image &other) const
    {
        // TODO 待优化
        return bm_orig == other.bm_orig;
    }

    void bitmapScale(double sc)
    {
        bm_orig = scaleImg(bm_orig, sc);
        bm_display = QImage();
    }

    void displayScale(double sc)
    {
        display_scale /= sc;
        bm_display = QImage();
    }

    void resetScale(double sc)
    {
        display_scale = sc;
        bm_display = QImage();
    }

    QImage &display()
    {
        if (bm_display.isNull())
        {
            bm_display = scaleImg(bm_orig, 1.0 / display_scale);
            // TODO
            // FIXME: this won't work because it will ignore the cell's bg color.
            //MakeInternallyScaled(bm_display, *wxWHITE, sys->frame->csf_orig);
        }
        return bm_display;
    }

    static QImage scaleImg(const QImage &src, double sc)
    {
        const auto &s = src.size() * sc;
        return src.scaled(s, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
};




#endif // IMAGE

