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
    void addFileToHistory(const QString &filename);


    void rememberOpenFiles();
    inline QStringList getOpenFiles() { return lastopenfile; }

private:
    void load();
    void save();
    void refreshMenu();

    QStringList lastopenfile;
    QStringList historyfile;
    QMenu*menu;
    bool dirty0, dirty1;
};

#endif // HISTORY_H
