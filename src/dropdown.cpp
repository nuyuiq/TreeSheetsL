#include "dropdown.h"
#include "config.h"
#include "myapp.h"
#include "tools.h"

#include <QDebug>
#include <QAbstractItemDelegate>
#include <QPainter>
#include <QDir>
#include <QPixmap>

static Color getColor(int idx)
{
    return Color(idx == CUSTOMCOLORIDX?
                      myApp.cfg->customcolor:
                      _g::celltextcolors[idx]);
}

class ItemDelegate : public QAbstractItemDelegate
{
public:
    explicit ItemDelegate(int tp, QObject *parent) : QAbstractItemDelegate(parent), tp(tp) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (tp == 0)
        {
            int idx = index.row();
            painter->setBrush(getColor(idx));
            painter->drawRect(option.rect);
            if (idx == CUSTOMCOLORIDX)
            {
                painter->setPen(Qt::black);
                auto font = painter->font();
                font.setPointSize(9);
                painter->setFont(font);
                painter->drawText(option.rect.adjusted(2, 0, 0, 0), QStringLiteral("Custom"));
            }
        }
        else
        {
            int h = qMin(option.rect.width(), option.rect.height());
            auto img = index.data(Qt::UserRole).value<QPixmap>();
            if (h != img.size().height())
            {
                img = img.scaledToHeight(h);
            }
            painter->drawPixmap(option.rect.x(), option.rect.y(), img);
        }
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
    {
        auto a = qobject_cast<QComboBox*>(option.widget->parent()->parent());
        return QSize(option.widget->width(), a->height());
    }
private:
    qint8 tp;
};

ColorDropdown::ColorDropdown(
        int sel, int width, int msc,
        QWidget *parent) : QComboBox(parent)
{
    setMinimumWidth(width);
    setMaximumWidth(width);
    setMaxVisibleItems(msc);
    setItemDelegate(new ItemDelegate(0, this));
    for (int i = 0; i < _g::celltextcolors_size; i++)
    {
        addItem(QString());
    }
    setCurrentIndex(sel);
    // 没有双击，用右键代替
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ColorDropdown::customContextMenuRequested, this, [this](){
        emit this->currentIndexChanged(currentIndex());
    });
//    connect(this, QOverload<int>::of(&ColorDropdown::currentIndexChanged), this, [this](){
//        qDebug() << this->currentText() << sender();
//    });
}

Color ColorDropdown::currentColor() const
{
    return getColor(currentIndex());
}

void ColorDropdown::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setBrush(currentColor());
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

ImageDropdown::ImageDropdown(int sel, int width, int msc, const QString &path, QWidget *parent) : QComboBox(parent)
{
    setMinimumWidth(width);
    setMaximumWidth(width);
    setMaxVisibleItems(msc);
    setItemDelegate(new ItemDelegate(1, this));
    const QStringList &ls = QDir(path).entryList(QStringList()<<QStringLiteral("*.*"), QDir::Files);
    foreach (const QString &imgfn, ls)
    {
        QPixmap img;
        if (img.load(path + "/" +imgfn))
        {
            // TODO 先缩放还是后缩放?
            addItem(QString(), img);
        }
    }
    setCurrentIndex(sel);
    // 没有双击，用右键代替
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ColorDropdown::customContextMenuRequested, this, [this](){
        emit this->currentIndexChanged(currentIndex());
    });
}

QPixmap ImageDropdown::currentImage() const
{
    return currentData().value<QPixmap>();
}

void ImageDropdown::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    int h = qMin(width(), height()) - 1;
    auto img = currentImage();
    if (h != img.size().height())
    {
        img = img.scaledToHeight(h);
    }
    p.drawRect(0, 0, width() - 1, height() - 1);
    p.drawPixmap(1, 1, img);
}
