#ifndef HISTORY_H
#define HISTORY_H

#include <QStringList>

class QMenu;

class FileHistory
{
public:
    FileHistory();
    ~FileHistory();

    void setMenu(QMenu*menu);

private:
    void load();
    void save();
    void refreshMenu();

    QStringList files;
    QMenu*menu;
    bool dirty;
};

#endif // HISTORY_H
