#include "myapp.h"
#include "mainwindow.h"
#include "symdef.h"
#include "tools.h"
#include "mylog.h"
#include "server.h"
#include "config.h"

#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <QApplication>

// 全局变量
MyApp myApp;

// 初始化
bool MyApp::Init()
{
    QString filename;
    // 是否以便携启动
    bool portable = false;
    // 是否单例程序模式的方式启动
    bool single_instance = true;

    for (const QString &str : QApplication::arguments().mid(1))
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





    frame = new MainWindow;
    frame->show();
    return true;
}

void MyApp::Relese()
{
    DELPTR(frame);
    DELPTR(cfg);
    DELPTR(log);
    DELPTR(server);
}

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
