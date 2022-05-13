#include "widget.h"
#include "document.h"
#include "tools.h"
#include "config.h"
#include "myapp.h"
#include "mainwindow.h"
#include "symdef.h"

#include <QScrollArea>
#include <QLabel>
#include <QVariant>
#include <QPainter>
#include <QDebug>
#include <QWheelEvent>
#include <QScrollBar>

Widget::Widget(QScrollArea *scroll) : scrollwin(scroll)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);

    doc = new Document(this);
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

void Widget::updateHover(int mx, int my, QPainter &dc)
{
    doc->Hover(mx / doc->currentviewscale, my / doc->currentviewscale, dc);
}

void Widget::cursorScroll(int dx, int dy)
{
//    auto p = pos() + QPoint(dx, dy);
//    qDebug() << pos() << p;
//    //move(p);
    if (dy)
    {
        auto bar = scrollwin->verticalScrollBar();
        bar->setValue(bar->value() + dy);
    }
    if (dx)
    {
        auto bar = scrollwin->horizontalScrollBar();
        bar->setValue(bar->value() + dx);
    }
    qDebug() << dx << dy;
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

void Widget::wheelEvent(QWheelEvent *e)
{
    //QWidget::wheelEvent(e);
    e->accept();

    bool ctrl  = e->modifiers() & Qt::CTRL;
    bool alt   = e->modifiers() & Qt::ALT;
    bool shift = e->modifiers() & Qt::SHIFT;
    if (myApp.cfg->zoomscroll) ctrl = !ctrl;


    if (alt || ctrl || shift)
    {
        int steps = e->delta() / WHEELDELTA;
        if (!steps) return;
        QPainter dc(this);
        updateHover(e->x(), e->y(), dc);
        status(doc->wheel(dc, steps, alt, ctrl, shift));
    }
    else
    {
        //QPainter dc();
        QPainter dc(this);
        if (e->orientation() == Qt::Horizontal)
        {
            cursorScroll(e->delta() * _g::scrollratewheel, 0);
            updateHover(e->x(), e->y(), dc);
        }
        else
        {
            cursorScroll(0, -e->delta() * _g::scrollratewheel);
            updateHover(e->x(), e->y(), dc);
        }
    }
}

