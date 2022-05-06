#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widget.h"
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

Widget *MainWindow::createWidget(const QString &name, bool append)
{
    QScrollArea*scroll = new QScrollArea;
    Q_ASSERT(scroll);
    Widget* widget = new Widget(scroll);
    Q_ASSERT(widget);
    ui->centralwidget->insertTab(append? -1: 0, scroll, name);
    return widget;
}
