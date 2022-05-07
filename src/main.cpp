#include "myapp.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    int ec = myApp.Init()? a.exec(): 0;
    myApp.Relese();
    return ec;
}
