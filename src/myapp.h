#ifndef MYAPP_H
#define MYAPP_H

#include <QCoreApplication>
#include "image.h"

// 声明
class MainWindow;
class MyLog;
class Server;
class Config;
class FileHistory;
class Cell;
class QString;



// 成员需使用非对象的简洁类型
struct MyApp
{
    MainWindow *frame;
    MyLog *log;
    Server *server;
    Config *cfg;
    FileHistory *fhistory;
    ImagesRef *imageRef;


    bool Init();
    void Relese();

    //! 加载本地文件
    QString LoadDB(const QString &filename, bool fromreload);

    //! 初始化空文档
    Cell *InitDB(int sizex, int sizey = 0);

    //! 更新全局图片缓存
    ImagePtr wrapImage(const Image &img);

    //! 加载翻译文件
    void loadTranslation(const QString &filename);
    // 定义 MyApp 内翻译助手
    Q_DECLARE_TR_FUNCTIONS(MyApp)
};

//! 全局参考，快速引用
extern MyApp myApp;

#endif // MYAPP_H
