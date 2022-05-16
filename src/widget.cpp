#include "widget.h"
#include "document.h"
#include "tools.h"
#include "config.h"
#include "myapp.h"
#include "mainwindow.h"
#include "symdef.h"
#include "cell.h"

#include <QScrollArea>
#include <QLabel>
#include <QVariant>
#include <QDebug>
#include <QWheelEvent>
#include <QScrollBar>
#include <QLineEdit>
#include <QMenu>

Widget::Widget(QScrollArea *scroll) : scrollwin(scroll)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMouseTracking(true);

    doc = new Document(this);
    lastrmbwaswithctrl = false;

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
}

void Widget::selectClick(int mx, int my, bool right, int isctrlshift)
{
    if (mx < 0 || my < 0)
    {
        // for some reason, using just the "menu" key sends a right-click at (-1, -1)
        return;
    }
    Tools::Painter dc(this);
    updateHover(mx, my, dc);
    doc->select(dc, right, isctrlshift);
}

QVariant Widget::inputMethodQuery(Qt::InputMethodQuery imq) const
{
    if (imq == Qt::ImCursorRectangle)
    {
        Cell* c = doc->selected.getCell();
        if (c && !c->tiny && (c->hasText() || !c->grid) && doc->selected.textEdit())
        {
            Tools::Painter dc(this);
            doc->shiftToCenter(dc);
            return dc.transform().mapRect(doc->cursorlastinfo);
        }
    }
    return QWidget::inputMethodQuery(imq);
}

void Widget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setClipRect(e->rect());
    doc->draw(painter);
}

void Widget::inputMethodEvent(QInputMethodEvent *e)
{
    QWidget::inputMethodEvent(e);
}

void Widget::wheelEvent(QWheelEvent *e)
{
    e->accept();
    bool ctrl  = e->modifiers() & Qt::ControlModifier;
    bool alt   = e->modifiers() & Qt::AltModifier;
    bool shift = e->modifiers() & Qt::ShiftModifier;
    if (myApp.cfg->zoomscroll) ctrl = !ctrl;


    if (alt || ctrl || shift)
    {
        int steps = e->delta() / WHEELDELTA;
        if (!steps) return;
        Tools::Painter dc(this);
        updateHover(e->x(), e->y(), dc);
        status(doc->wheel(dc, steps, alt, ctrl, shift));
    }
    else
    {
        Tools::Painter dc(this);
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

void Widget::mousePressEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
    {
        setFocus();
        if (e->modifiers() & Qt::ShiftModifier) mouseMoveEvent(e);
        else
        {
            int mk = 0;
            if (e->modifiers() & Qt::ControlModifier) mk += 1;
            if (e->modifiers() & Qt::AltModifier) mk += 2;
            selectClick(e->x(), e->y(), false, mk);
        }
    }
    else if (e->buttons() & Qt::RightButton)
    {
        setFocus();
        selectClick(e->x(), e->y(), true, 0);
        lastrmbwaswithctrl = e->modifiers() & Qt::ControlModifier;
    }
}

void Widget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
    {
        if (e->modifiers() & (Qt::ControlModifier|Qt::AltModifier))
        {
            doc->selectUp();
        }
    }
}

void Widget::mouseMoveEvent(QMouseEvent *e)
{
    Tools::Painter dc(this);
    updateHover(e->x(), e->y(), dc);
    if (e->buttons() & (Qt::LeftButton | Qt::RightButton))
    {
        // TODO
        // doc->Drag(dc);
    }
    else if (e->buttons() & Qt::MiddleButton)
    {
        const QPoint &p = e->pos() - lastmousepos;
        cursorScroll(-p.x(), -p.y());
    }
    lastmousepos = e->pos();
}

void Widget::mouseDoubleClickEvent(QMouseEvent *e)
{
    Tools::Painter dc(this);
    updateHover(e->x(), e->y(), dc);
    doc->doubleClick(dc);
    setFocus();
    status(QString());
}

void Widget::contextMenuEvent(QContextMenuEvent *e)
{
    if (lastrmbwaswithctrl)
    {
        if (doc->tags.isEmpty()) return;
        QMenu tagmenu;
        const auto begin = doc->tags.constBegin();
        const auto end = doc->tags.constEnd();
        for (auto itr = begin; itr != end; itr++)
        {
            myApp.frame->appendSubMenu(&tagmenu, A_TAGSET + (itr - begin), *itr, QString(), false);
        }
        tagmenu.exec(e->globalPos());
    }
    else
    {
        myApp.frame->editmenupopup->exec(e->globalPos());
    }
}
