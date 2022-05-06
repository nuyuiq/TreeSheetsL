#include "myapp.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MyApp myApp;
    return a.exec();
}
