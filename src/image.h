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

//typedef QImage Image;
class Image : public QImage {
public:
    Image(const QImage &other, double sc)
        : QImage(other), display_scale(sc) {}
    inline bool operator==(const QImage &other) const
    {
        // TODO 待优化
        return QImage::operator==(other);
    }

    double display_scale;
};




#endif // IMAGE

