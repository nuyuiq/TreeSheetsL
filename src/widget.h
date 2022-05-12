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
    QScrollArea *scroll;
    int mousewheelaccum;
    bool lastrmbwaswithctrl;

protected:
    void paintEvent(QPaintEvent *);
    void inputMethodEvent(QInputMethodEvent *);
};



#endif // WIDGET_H
