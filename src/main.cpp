#include "myapp.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    int ec = myApp.init()? a.exec(): 0;
    myApp.relese();
    return ec;
}
