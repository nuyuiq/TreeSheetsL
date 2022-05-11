#include "widget.h"
#include "document.h"
#include "tools.h"
#include "myapp.h"
#include "mainwindow.h"

#include <QScrollArea>
#include <QLabel>

Widget::Widget(QScrollArea *scroll) : scroll(scroll)
{
    Q_ASSERT(scroll);
    scroll->setWidget(this);
    doc = new Document(this);
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
