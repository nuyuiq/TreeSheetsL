#include "widget.h"
#include "document.h"
#include "tools.h"
#include "myapp.h"
#include "mainwindow.h"

#include <QScrollArea>
#include <QLabel>
#include <QVariant>
#include <QPainter>
#include <QDebug>

Widget::Widget(QScrollArea *scroll) : scroll(scroll)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);

    doc = new Document(this);
    mousewheelaccum = 0;
    lastrmbwaswithctrl = false;
    // TODO
//    DisableKeyboardScrolling();
//    // Without this, ScrolledWindow does its own scrolling upon mousewheel events, which
//    // interferes with our own.
//    EnableScrolling(false, false);

    Q_ASSERT(scroll);
    scroll->setWidget(this);
}

Widget::~Widget()
{
    DELPTR(doc);
}

void Widget::status(const QString &msg)
{
    if (auto l = myApp.frame->sbl[0])
    {
        l->setText(msg);
    }
}

QVariant Widget::inputMethodQuery(Qt::InputMethodQuery imq) const
{
    return QWidget::inputMethodQuery(imq);
}

void Widget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    doc->draw(painter);
}

void Widget::inputMethodEvent(QInputMethodEvent *e)
{
    QWidget::inputMethodEvent(e);
}
