#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "selection.h"

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QCoreApplication>
#include <QRect>

class Widget;
struct Cell;
class QString;
class QPainter;
class Selection;

struct UndoItem
{
    QVector<Selection> path, selpath;
    Selection sel;
    Cell* clone;


    inline UndoItem() : clone(nullptr) { }
    ~UndoItem();
};

struct Document
{
    QString filename;
    QStringList tags;
    QDateTime lastmodificationtime;
    QVector<Selection> drawpath;
    QVector<Cell *> itercells;
    QList<UndoItem *> undolist, redolist;
    QRect cursorlastinfo;
    Selection hover;
    Selection selected;
    Selection begindrag;
    Widget *sw;
    Cell *rootgrid;
    // for use during Render() calls
    Cell *curdrawroot;

    double currentviewscale;
    long lastmodsinceautosave;
    long undolistsizeatfullsave;
    long lastsave;
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
    int isctrlshiftdrag;
    int editfilter;
    uint printscale;
    bool modified;
    bool tmpsavesuccess;
    bool redrawpending;
    bool while_printing;
    bool scaledviewingmode;
    bool blink;
    bool searchfilter;




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
    void Hover(int x, int y, QPainter &dc);
    void select(QPainter &dc, bool right, int isctrlshift);
    void selectUp();
    void doubleClick(QPainter &dc);
    void refreshHover();
    void zoom(int dir, QPainter &dc, bool fromroot = false, bool selectionmaybedrawroot = true);
    void createPath(Cell *c, QVector<Selection> &path);
    void Blink();
    void drag(QPainter &dc);
    bool lastUndoSameCell(Cell *c) const;
    QString save(bool saveas, bool *success = nullptr);
    void autoSave(bool minimized, int page);
    QString saveDB(bool *success, bool istempfile = false, int page = -1);
    QString exportFile(const QString &fn, int k, bool currentview);
    QString Export(const QString &fmt, const QString &pat, const QString &msg, int k);
    bool closeDocument();
    bool checkForChanges();
    QString action(QPainter &dc, int k);
    QString tagSet(int tagno);
    void undo(QPainter &dc, QList<UndoItem *> &fromlist, QList<UndoItem *> &tolist, bool redo = false);
    void scrollOrZoom(QPainter &dc, bool zoomiftiny = false);
    void zoomTiny(QPainter &dc);
    uint pickColor(QWidget *fr, uint defcol);
    void collectCells(Cell *c);
    void collectCellsSel(bool recurse);
    QString searchNext(QPainter &dc);
    void applyEditFilter();
    void setSearchFilter(bool on);
    void delRowCol(int &v, int e, int gvs, int dec, int dx, int dy, int nxs, int nys);
    void zoomOutIfNoGrid(QPainter &dc);
    QString sort(bool descending);
    QString layrender(int ds, bool vert, bool toggle = false, bool noset = false);


    inline void resetCursor() { if (selected.g) selected.setCursorEdit(this, selected.textEdit()); }
    inline void refreshReset() { refresh(); }
    // 状态栏提示语
    inline QString noSel() const { return tr("This operation requires a selection."); }
    inline QString oneCell() const { return tr("This operation works on a single selected cell only."); }
    inline QString noThin()  const { return tr("This operation doesn't work on thin selections."); }
    inline QString noGrid()  const { return tr("This operation requires a cell that contains a grid."); }
    // 定义 Document 内翻译助手
    Q_DECLARE_TR_FUNCTIONS(Document)
};



#endif // DOCUMENT_H
