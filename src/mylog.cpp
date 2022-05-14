#include "mylog.h"

#include <QDebug>
#include <QLocale>

static QtMessageHandler oldHandler;

static void msgOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // todo
    //const QString &output = qFormatLogMessage(type, context, msg);

    if (Q_LIKELY(oldHandler)) oldHandler(type, context, msg);
}

MyLog::MyLog()
{
    oldHandler = qInstallMessageHandler(msgOutput);
    const QString &pattern =
        #ifdef QT_DEBUG
            QStringLiteral("[%{time mm:ss.zzz} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{file}:%{line} %{function} -- %{message}");
        #else
            QStringLiteral("[%{time mm:ss.zzz} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] -- %{message}");
        #endif
    qSetMessagePattern(pattern);

    showInfo();
}

void MyLog::showInfo()
{
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "Sys Locale:" << QLocale::system().name();
}


