#include "widget.h"
#include "document.h"
#include "tools.h"

#include <QScrollArea>

Widget::Widget(QScrollArea *scroll) : scroll(scroll)
{
    Q_ASSERT(scroll);
    scroll->setWidget(this);
    doc = new Document;
}

Widget::~Widget()
{
    DELPTR(doc);

}
