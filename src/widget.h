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
    void updateHover(int mx, int my, QPainter &dc);
    void cursorScroll(int dx, int dy);
    void selectClick(int mx, int my, bool right, int isctrlshift);


    Document *doc;
    QScrollArea *scrollwin;

protected:
    void paintEvent(QPaintEvent *);
    void inputMethodEvent(QInputMethodEvent *);
    void wheelEvent(QWheelEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void contextMenuEvent(QContextMenuEvent *);
    void keyPressEvent(QKeyEvent *);
    void dropEvent(QDropEvent *);
    void dragEnterEvent(QDragEnterEvent *);
    void dragMoveEvent(QDragMoveEvent *);
private:
    QPoint lastmousepos;    

};



#endif // WIDGET_H
