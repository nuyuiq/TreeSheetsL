#ifndef DROPDOWN_H
#define DROPDOWN_H

#include <QComboBox>

class ColorDropdown : public QComboBox
{
public:
    explicit ColorDropdown(int sel, int width, int msc, QWidget *parent = nullptr);
    QColor currentColor() const;

protected:
    void paintEvent(QPaintEvent *);
};

class ImageDropdown : public QComboBox
{
public:
    explicit ImageDropdown(int sel, int width, int msc, const QString &path, QWidget *parent = nullptr);
    QPixmap currentImage() const;

protected:
    void paintEvent(QPaintEvent *);
};

#endif // DROPDOWN_H
