#include "widget.h"
#include <QScrollArea>

Widget::Widget(QScrollArea *scroll) : scroll(scroll)
{
    Q_ASSERT(scroll);
    scroll->setWidget(this);

}
