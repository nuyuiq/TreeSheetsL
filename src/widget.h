#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class QScrollArea;
struct Document;

class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QScrollArea *scroll);
    ~Widget();

    Document *doc;
    QScrollArea *scroll;
};

#endif // WIDGET_H
