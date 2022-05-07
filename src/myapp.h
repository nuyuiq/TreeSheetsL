#ifndef MYAPP_H
#define MYAPP_H

// 声明
class MainWindow;
class MyLog;
class Server;
class Config;

class QString;

// 成员需使用非对象的简洁类型
struct MyApp
{
    MainWindow *frame;
    MyLog *log;
    Server *server;
    Config *cfg;


    bool Init();
    void Relese();
    //! 加载翻译文件
    void loadTranslation(const QString &filename);
};

//! 全局参考，快速引用
extern MyApp myApp;

#endif // MYAPP_H
