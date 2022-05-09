#include "tools.h"

#include <QApplication>
#include <QFileInfo>

QString Tools::resolvePath(const QString &path, bool exist)
{
    QString sp = path.isEmpty()? QStringLiteral("."): path;
    bool isrp =
#ifdef Q_OS_WIN
    sp.length() > 2 && sp.at(1) == QChar::fromLatin1(':');
#else
    sp.at(0) == "/";
#endif
    if (!isrp) sp = QApplication::applicationDirPath() + "/" + sp;
    if (exist && !QFileInfo::exists(sp)) sp.clear();
    return sp;
}

QString Tools::bakName(const QString &filename)
{
    return extName(filename, QStringLiteral(".bak"));
}

QString Tools::tmpName(const QString &filename)
{
    return extName(filename, QStringLiteral(".tmp"));
}

QString Tools::extName(const QString &filename, const QString &ext)
{
    return QFileInfo(filename).filePath() + ext;
}
