#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "selection.h"

#include <QString>
#include <QMap>
#include <QDateTime>
#include <QVector>

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
    Cell *rootgrid;
    // for use during Render() calls
    Cell *curdrawroot;
    Selection hover;
    Selection selected;
    QVector<Selection> drawpath;

    double currentviewscale;
    long lastmodsinceautosave;
    long undolistsizeatfullsave;
    //long lastsave;
    int originx;
    int originy;
    int maxx;
    int maxy;
    int centerx;
    int centery;

    int layoutxs;
    int layoutys;
    int hierarchysize;
    int fgutter;
    int lasttextsize;
    int laststylebits;
    int pathscalebias;
    bool modified;
    //bool tmpsavesuccess;
    bool redrawpending;
    bool while_printing;
    bool scaledviewingmode;




    Document(Widget *sw);
    ~Document();
    void initWith(Cell *r, const QString& filename);
    void clearSelectionRefresh();
    bool scrollIfSelectionOutOfView(QPainter &dc, Selection &s, bool refreshalways = false);
    void drawSelect(QPainter &dc, Selection &s, bool refreshinstead = false, bool cursoronly = false);
    void drawSelectMove(QPainter &dc, Selection &s, bool refreshalways = false,
                        bool refreshinstead = true);
    void draw(QPainter &dc);
    void layout(QPainter &dc);
    void shiftToCenter(QPainter &dc);
    void render(QPainter &dc);
    void refresh();
    void resetFont();
    void addUndo(Cell *c);
    void changeFileName(const QString &fn, bool checkext);
    void updateFileName(int page = -1);
    uint background() const;
    Cell *walkPath(const QVector<Selection> &path);
    bool pickFont(QPainter &dc, int depth, int relsize, int stylebits);
    int textSize(int depth, int relsize);
    bool fontIsMini(int textsize) const;
    const QString wheel(QPainter &dc, int dir, bool alt, bool ctrl, bool shift, bool hierarchical = true);
    inline void refreshReset() { refresh(); }
    void Hover(int x, int y, QPainter &dc);
};



#endif // DOCUMENT_H
