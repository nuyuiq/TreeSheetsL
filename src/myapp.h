#ifndef MYAPP_H
#define MYAPP_H

// 声明
class MainWindow;
class QString;

struct MyApp
{
    MainWindow*frame;




    MyApp();
    ~MyApp();
    //! 加载翻译文件
    void loadTranslation(const QString &filename);
    QString GetDocPath(const QString &relpath);
    QString GetDataPath(const QString &relpath);

};

//! 全局参考，快速引用
extern MyApp*myApp;

#endif // MYAPP_H
