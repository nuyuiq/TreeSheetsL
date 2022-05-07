#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Widget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //! 创建新页面
    Widget *createWidget(const QString &name = QString(), bool append = false);

private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);


private:
    void initUI();

    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QMenu *editmenupopup;
};
#endif // MAINWINDOW_H
