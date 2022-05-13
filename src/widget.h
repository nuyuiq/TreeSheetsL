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
    QVariant inputMethodQuery(Qt::InputMethodQuery) const;

    void status(const QString &msg);

    Document *doc;
    QScrollArea *scrollwin;
    bool lastrmbwaswithctrl;

    void updateHover(int mx, int my, QPainter &dc);

    void cursorScroll(int dx, int dy);
protected:
    void paintEvent(QPaintEvent *);
    void inputMethodEvent(QInputMethodEvent *);
    void wheelEvent(QWheelEvent *);

};



#endif // WIDGET_H
