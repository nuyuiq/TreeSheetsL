#include "myapp.h"
#include "mainwindow.h"
#include <QLocale>
#include <QDir>
#include <QTranslator>
#include <QStringList>
#include <QApplication>
#include <QDebug>

// 全局参考，快速引用
MyApp*myApp;

//! 检查路径是否存在，存在返回
static QString CheckPath(const QStringList &candidatePaths)
{
    for (auto &path : candidatePaths)
    {
        if (QDir(path). exists())
        {
            return path;
        }
    }
    return QString();
}

QString MyApp::GetDocPath(const QString &relpath)
{
    QStringList ls;
    ls << QApplication::applicationDirPath() + "/" + relpath;
#ifdef TREESHEETS_DOCDIR
    ls << QString(TREESHEETS_DOCDIR "/") + relpath;
#endif
    return CheckPath(ls);
}

QString MyApp::GetDataPath(const QString &relpath)
{
    QStringList ls;
    ls << QApplication::applicationDirPath() + "/" + relpath;
#ifdef TREESHEETS_DATADIR
    ls << QString(TREESHEETS_DATADIR "/") + relpath;
#endif
    return CheckPath(ls);
}

MyApp::MyApp()
{
    myApp = this;
    loadTranslation("ts");
    frame = new MainWindow;


    frame->show();
}

MyApp::~MyApp()
{
    delete frame;
    myApp = nullptr;
}

void MyApp::loadTranslation(const QString &filename)
{
    const QString basepath = GetDataPath("translations");
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
