#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "image.h"

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QScopedPointer>

class Widget;
class QActionGroup;
class QLineEdit;
class ColorDropdown;
class ImageDropdown;
class QFileSystemWatcher;
class QLabel;
class Selection;
class Cell;
class QPrinter;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //! 创建新页面
    Widget *createWidget(bool append = false);
    //! 必须是文件，一次性，被触发后须重复
    void fileChangeWatch(const QString &file);
    //! 从一打开的文档中选中对应名字的画板
    Widget *getTabByFileName(const QString &fn);
    //! 仅返回面板，不切换
    Widget *getTabByIndex(int i) const;
    //! 当前页
    Widget *getCurTab();
    //! 标题
    void setPageTitle(const QString &fn, const QString &mods, int page = -1);
    void appendSubMenu(
            QMenu *menu, int tag, const QString &contents,
            const QString &help=QString(),
            bool general = true,
            int status=-1,
            QActionGroup *group=nullptr);
    //! 更新选中状态在状态栏
    void updateStatus(const Selection &s);
    //! 解除图标状态
    void deIconize();
    void tabsReset();
    void cycleTabs(int offset = 1);
    QPrinter *getPrinter();


    ImagePtr foldicon;
    ImagePtr line_nw;
    ImagePtr line_sw;
    QTabWidget*nb;
    QSystemTrayIcon *trayIcon;
    QMenu *editmenupopup;
    QLineEdit *filter, *replaces;
    QFileSystemWatcher *watcher;
    QLabel*sbl[4];
    QToolBar *tb;
    QStatusBar *sb;
    ColorDropdown *celldd, *textdd, *borddd;
    ImageDropdown *idd;
    QPrinter *printer;
    QScopedPointer<Cell> cellclipboard;
    QString searchstring;
    QString clipboardcopy;
    bool fromclosebox;
    bool zenmode;



private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void actionActivated();
    void fileChanged(const QString &path);
    void tabClose(int idx);
    void tabChange(int idx);

private:
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *event);
    void initUI();
    void createShortcut(const QVariant &key, int kid);

    //! 编辑光标定时器
    int blinkTimer;
    //! 定时保存检查定时器
    int savechecker;


    QStringList shortcut_bak;

};
#endif // MAINWINDOW_H
