#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QMap>
#include <QDateTime>

class Widget;
struct Cell;
class QString;
class QPainter;
class Selection;

struct Document
{
    QString filename;
    QMap<QString, bool> tags;
    QDateTime lastmodificationtime;
    Widget *sw;

    //long lastmodsinceautosave;
    long undolistsizeatfullsave;
    //long lastsave;
    bool modified;
    //bool tmpsavesuccess;



    Document(Widget *sw);
    ~Document();
    void initWith(Cell *r, const QString& filename);
    void clearSelectionRefresh();
    bool scrollIfSelectionOutOfView(QPainter &dc, Selection &s, bool refreshalways = false);
    void drawSelect(QPainter &dc, Selection &s, bool refreshinstead = false, bool cursoronly = false);
    void drawSelectMove(QPainter &dc, Selection &s, bool refreshalways = false,
                        bool refreshinstead = true);
    void refresh();
    void resetFont();
    void addUndo(Cell *c);


    inline void refreshReset() { refresh(); }
};



#endif // DOCUMENT_H
