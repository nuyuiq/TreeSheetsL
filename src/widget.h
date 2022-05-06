#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class QScrollArea;

class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QScrollArea *scroll);



private:
    QScrollArea *scroll;
};

#endif // WIDGET_H
