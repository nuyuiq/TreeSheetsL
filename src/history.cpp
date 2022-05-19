#include "history.h"
#include "myapp.h"
#include "config.h"
#include "symdef.h"
#include "mainwindow.h"
#include "widget.h"
#include "document.h"

#include <QDebug>
#include <QMenu>

FileHistory::FileHistory()
{
    menu = nullptr;
    load();
}

FileHistory::~FileHistory()
{
    save();
}

void FileHistory::setMenu(QMenu *menu)
{
    this->menu = menu;
    refreshMenu();
}

void FileHistory::addFileToHistory(const QString &filename)
{
    if (historyfile.size() && historyfile.at(0) == filename) return;

    historyfile.removeOne(filename);
    historyfile.prepend(filename);
    while (historyfile.size() > MAXCOUNT_HISTORY)
    {
        historyfile.removeLast();
    }
    dirty0 = true;
    refreshMenu();
}

QString FileHistory::getHistoryFile(int idx)
{
    Q_ASSERT(idx >= 0);
    if (idx >= historyfile.size()) return QString();
    return historyfile.at(idx);
}

void FileHistory::rememberOpenFiles()
{
    int n = myApp.frame->nb->count();
    QStringList fs;
    for (int i = 0; i < n; i++)
    {
        auto *p = myApp.frame->getTabByIndex(i);
        if (!p->doc->filename.isEmpty())
        {
            fs << p->doc->filename;
        }
    }
    if (lastopenfile != fs)
    {
        lastopenfile = fs;
        dirty1 = true;
    }
}

void FileHistory::load()
{
    const QString fm(QStringLiteral("file%1"));
    historyfile.clear();
    for (int i = 1; i <= MAXCOUNT_HISTORY; i++)
    {
        const auto fn = myApp.cfg->read(fm.arg(i)).toString();
        if (fn.isEmpty()) break;
        if (!historyfile.contains(fn)) historyfile << fn;
    }
    dirty0 = false;
    lastopenfile.clear();
    int numfiles = myApp.cfg->read(QStringLiteral("numopenfiles"), 0).toInt();
    for (int i = 0; i < numfiles; i++)
    {
        QString fn = QStringLiteral("lastopenfile_%1").arg(i);
        fn = myApp.cfg->read(fn, i).toString();
        if (!fn.isEmpty()) lastopenfile << fn;
    }
    dirty1 = false;
    refreshMenu();
}

void FileHistory::save()
{
    if (dirty0)
    {
        const QString fm(QStringLiteral("file%1"));
        for (int i = 0; i < historyfile.size(); i++)
        {
            myApp.cfg->write(fm.arg(i + 1), historyfile.at(i));
        }
        for (int i = historyfile.size(); i < MAXCOUNT_HISTORY; i++)
        {
            const auto key = fm.arg(i + 1);
            if (!myApp.cfg->read(key).toString().isEmpty())
            {
                myApp.cfg->write(key, QVariant());
            }
        }
        dirty0 = false;
    }
    if (dirty1)
    {
        for (int i = 0; i < lastopenfile.size(); i++)
        {
            myApp.cfg->write(QStringLiteral("lastopenfile_%1").arg(i), lastopenfile.at(i));
        }
        myApp.cfg->write(QStringLiteral("numopenfiles"), lastopenfile.size());
        dirty1 = false;
    }
}

void FileHistory::refreshMenu()
{
    if (menu == nullptr) return;
    menu->clear();
    for (int i = 0; i < historyfile.size(); i++)
    {
        myApp.frame->appendSubMenu(menu, A_FILEHIS0 + i, historyfile.at(i), QString(), false);
    }
}
