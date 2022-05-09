#include "history.h"
#include "myapp.h"
#include "config.h"
#include "symdef.h"
#include "mainwindow.h"

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

void FileHistory::load()
{
    const QString fm(QStringLiteral("file%1"));
    files.clear();
    for (int i = 1; i <= MAXCOUNT_HISTORY; i++)
    {
        const auto fn = myApp.cfg->read(fm.arg(i)).toString();
        if (fn.isEmpty()) break;
        files << fn;
    }
    dirty = false;
    refreshMenu();
}

void FileHistory::save()
{
    if (!dirty) return;
    const QString fm(QStringLiteral("file%1"));
    for (int i = 1; i <= files.size(); i++)
    {
        myApp.cfg->write(fm.arg(i), files.at(i));
    }
    if (files.size() >= MAXCOUNT_HISTORY) return;
    const auto key = fm.at(files.size() + 1);
    if (!myApp.cfg->read(key).toString().isEmpty())
    {
        myApp.cfg->write(key, QVariant());
    }
}

void FileHistory::refreshMenu()
{
    if (menu == nullptr) return;
    menu->clear();
    for (int i = 0; i < files.size(); i++)
    {
        myApp.frame->appendSubMenu(menu, A_FILEHIS0 + i, files.at(i), QString(), false);
    }
}
