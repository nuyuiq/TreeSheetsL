#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

class Widget;
class QActionGroup;
class QLineEdit;
class ColorDropdown;
class ImageDropdown;

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
    void actionActivated();

private:
    void initUI();
    void appendSubMenu(
            QMenu *menu, int tag, const QString &contents,
            const QString &help=QString(),
            int status=-1,
            QActionGroup *group=nullptr);
    void createShortcut(const QVariant &key, int kid);

    QTabWidget*nb;
    QSystemTrayIcon *trayIcon;
    QMenu *editmenupopup;
    QLineEdit *filter, *replaces;
    ColorDropdown *celldd, *textdd, *borddd;
    ImageDropdown *idd;
};
#endif // MAINWINDOW_H
