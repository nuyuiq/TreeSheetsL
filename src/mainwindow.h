#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

class Widget;
class QActionGroup;
class QLineEdit;
class ColorDropdown;
class ImageDropdown;
class QFileSystemWatcher;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //! 创建新页面
    Widget *createWidget(bool append = false);

    void appendSubMenu(
            QMenu *menu, int tag, const QString &contents,
            const QString &help=QString(),
            bool general = true,
            int status=-1,
            QActionGroup *group=nullptr);

    //! 必须是文件，一次性，被触发后须重复
    void fileChangeWatch(const QString &file);
    //! 从一打开的文档中选中对应名字的画板
    Widget *getTabByFileName(const QString &fn);
    //! 仅返回面板，不切换
    Widget *getTabByIndex(int i) const;


    QTabWidget*nb;
    QSystemTrayIcon *trayIcon;
    QMenu *editmenupopup;
    QLineEdit *filter, *replaces;
    QFileSystemWatcher *watcher;
    QLabel*sbl[4];
    ColorDropdown *celldd, *textdd, *borddd;
    ImageDropdown *idd;



private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void actionActivated();
    void fileChanged(const QString &path);

private:
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *event);
    void initUI();
    void createShortcut(const QVariant &key, int kid);

    //! 编辑光标定时器
    int blinkTimer;
    //! 定时保存检查定时器
    int savechecker;


    QStringList *_tmp_shortcut;

};
#endif // MAINWINDOW_H
