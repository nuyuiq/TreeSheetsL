#include "myapp.h"
#include "mainwindow.h"
#include "symdef.h"
#include "tools.h"
#include "mylog.h"
#include "server.h"
#include "config.h"
#include "history.h"

#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <QApplication>
#include <QFile>
#include <QMessageBox>

// 全局变量
MyApp myApp;

//! 加载示例、教程
static void LoadTut()
{
    const auto &lang = QLocale::system().name().left(2);
    QString fn = Tools::resolvePath(QStringLiteral("examples/tutorial-%1.cts").arg(lang), false);
    if (!myApp.LoadDB(fn, false).isEmpty())
    {
        myApp.LoadDB(Tools::resolvePath(QStringLiteral("examples/tutorial.cts"), false), false);
    }
}

// 启动时初始化打开文档
static void docInit(const QString &filename)
{
    // 启动请求打开文档
    if (!filename.isEmpty())
    {
        myApp.LoadDB(filename, false);
    }

    if (!myApp.frame->nb->count())
    {
        int numfiles = myApp.cfg->read(QStringLiteral("numopenfiles"), 0).toInt();
        for (int i = 0; i < numfiles; i++)
        {
            QString fn = QStringLiteral("lastopenfile_%d").arg(i);
            fn = myApp.cfg->read(fn, i).toString();
            if (!fn.isEmpty()) myApp.LoadDB(fn, false);
        }
    }

    if (!myApp.frame->nb->count()) LoadTut();

    if (!myApp.frame->nb->count()) myApp.InitDB(10);


    // ScriptInit(frame);
}

// 初始化
bool MyApp::Init()
{
    QString filename;
    // 是否以便携启动
    bool portable = false;
    // 是否单例程序模式的方式启动
    bool single_instance = true;

    foreach (const QString &str, QApplication::arguments().mid(1))
    {
        if (str.startsWith(QChar::fromLatin1('-')))
        {
            const char ch = str.length() > 1? str.at(1).toLatin1(): 0;
            if (ch == 'p') portable = true;
            else if (ch == 'i') single_instance = false;
        }
        else
        {
            filename = str;
        }
    }

    // 单例模式处理
    bool isok;
    server = new Server(!single_instance, &isok);
    if (single_instance && !isok)
    {
        const QString msg = filename.isEmpty() ?
                    QString::fromLatin1("*"):
                    filename;
        server->sendMsgToAnother(msg);
        qDebug() << "Instance already running, sendMsg:" << msg;
        return false;
    }

    // 日志
    log = new MyLog;

    // 翻译
    loadTranslation("ts");

    // 配置文件
    cfg = new Config(portable);

    fhistory = new FileHistory;

    // [.frame]内部赋值，初始化过程中提前用到
    new MainWindow;

    docInit(filename);

    return true;
}

void MyApp::Relese()
{
    DELPTR(frame);
    DELPTR(fhistory);
    DELPTR(cfg);
    DELPTR(log);
    DELPTR(server);
}

// 加载本地文件
QString MyApp::LoadDB(const QString &filename, bool fromreload)
{
    // TODO
    QString fn = filename;
    bool loadedfromtmp = false;

    if (!fromreload)
    {
        //"this file is already loaded";
        if (frame->getTabByFileName(filename)) return QString();
        const QString tfn = Tools::tmpName(filename);
        if (QFile::exists(tfn))
        {
            int ret = QMessageBox::question(
                        frame,
                        QApplication::translate("MsgBox", "Autosave load"),
                        QApplication::translate("MsgBox", "A temporary autosave file exists, would you like to load it instead?"),
                        QMessageBox::Yes, QMessageBox::No);
            if (ret == QMessageBox::Yes)
            {
                fn = tfn;
                loadedfromtmp = true;
            }
        }
    }

    // todo






    return QString();
}

Cell *MyApp::InitDB(int sizex, int sizey)
{
    // TODO

    return nullptr;
//    Cell *c = new Cell(nullptr, nullptr, CT_DATA, new Grid(sizex, sizey ? sizey : sizex));
//    c->cellcolor = 0xCCDCE2;
//    c->grid->InitCells();
//    Document *doc = NewTabDoc();
//    doc->InitWith(c, L"");
//    return doc->rootgrid;
}

// 加载翻译文件
void MyApp::loadTranslation(const QString &filename)
{
    const QString basepath = Tools::resolvePath(TR_FILEPATH, true);
    if (basepath.isEmpty()) return;

    QTranslator *translator = new QTranslator;
    if (translator->load(QLocale::system(), filename, "_", basepath, ".qm"))
    {
        QApplication::installTranslator(translator);
        qDebug() << "load translator: " << filename << QLocale::system().name();
    }
    else
    {
        delete translator;
    }
}
