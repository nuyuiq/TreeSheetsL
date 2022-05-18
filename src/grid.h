#ifndef GRID_H
#define GRID_H

#include "selection.h"
#include "image.h"

#include <QVariantMap>

struct Cell;
namespace Tools { class DataIO; }
class QPainter;

struct Grid
{
    inline Cell *&C(int x, int y) const
    {
        Q_ASSERT(x >= 0 && y >= 0 && x < xs && y < ys);
        return cells[x + y * xs];
    }
    // owning cell.
    Cell *cell;
    // subcells
    Cell **cells;
    // widths for each column
    int *colwidths;
    int xs;
    int ys;

    int view_margin;
    int view_grid_outer_spacing;
    int user_grid_outer_spacing;
    int cell_margin;

    int bordercolor;
    bool horiz;
    bool folded;
    bool tinyborder;


    Grid(int _xs, int _ys, Cell *_c = nullptr);
    ~Grid();
    bool loadContents(Tools::DataIO &dis, int &numcells, int &textbytes, QVariantMap &info);
    void save(Tools::DataIO &dos, const QVector<ImagePtr> &imgs) const;
    void initColWidths();
    void setOrient();
    Selection findCell(Cell *o);
    void move(int dx, int dy, Selection &s);
    void drawSelect(Document *doc, QPainter &dc, Selection &s, bool cursoronly);
    void insertCells(int dx, int dy, int nxs, int nys, Cell *nc = nullptr);
    int minRelsize(int rs);
    void resetChildren();
    bool layout(Document *doc, QPainter &dc, int depth, int &sx, int &sy, int startx, int starty, bool forcetiny);
    void render(Document *doc, int bx, int by, QPainter &dc, int depth);
    void drawHover(Document *doc, QPainter &dc, Selection &s);
    void drawInsert(Document *doc, QPainter &dc, Selection &s, uint colour);
    void findXY(Document *doc, int px, int py, QPainter &dc);
    void resizeColWidths(int dir, const Selection &s, bool hierarchical);
    void relSize(int dir, int zoomdepth);
    void relSize(int dir, Selection &s, int zoomdepth);
    void multiCellDeleteSub(Document *doc, Selection &s);
    void delSelf(Document *doc, Selection &s);
    void deleteCells(int dx, int dy, int nxs, int nys);
    //! Clones g into this grid. This mutates the grid this function is called on.
    void clone(Grid *g);
    void mergeWithParent(Grid *p, Selection &s);
    QRect getRect(Document *doc, Selection &s, bool minimal = false);
    void drawCursor(Document *doc, QPainter &dc, Selection &s, uint color, bool cursoronly);
    void initCells(Cell *clonestylefrom = nullptr);
    Cell* cloneSel(const Selection &s);
    void replaceCell(Cell *o, Cell *n);
    void imageRefCollect(QVector<ImagePtr> &imgs);
    void maxDepthLeaves(int curdepth, int &maxdepth, int &leaves);
    QString convertToText(const Selection &s, int indent, int format, Document *doc);
    void collectCells(QVector<Cell *> &itercells);
    void collectCellsSel(QVector<Cell *> &itercells, Selection &s, bool recurse);
    Cell *findNextSearchMatch(const QString &search, Cell *best, Cell *selected, bool &lastwasselected);
    void findReplaceAll(const QString &str);
    void multiCellDelete(Document *doc, Selection &s);
    void setStyle(Document *doc, Selection &s, int sb);
    void setStyles(Selection &s, Cell *o);
    void clearImages(Selection &s);
    //! 不可重入
    void sort(Selection &s, bool descending);
    void replaceStr(Document *doc, const QString &str, Selection &s);
    void setBorder(int width, Selection &s);
    void setGridTextLayout(int ds, bool vert, bool noset, const Selection &s);
    void transpose();
    bool isTable();
    void hierarchify(Document *doc);
    void mergeRow(Grid *tm);
    void reParent(Cell *p);
    int flatten(int curdepth, int cury, Grid *g);
    Selection hierarchySwap(const QString &tag);
    void CSVImport(const QStringList &as, char sep);
    Cell *findLink(Selection &s, Cell *link, Cell *best, bool &lastthis, bool &stylematch, bool forward);
    Cell *findExact(const QString &s);
    Cell *deleteTagParent(Cell *tag, Cell *basecell, Cell *found);
    void mergeTagCell(Cell *f, Cell *&selcell);
    void mergeTagAll(Cell *into);
    void add(Cell *c);




    inline Selection selectAll() { return Selection(this, 0, 0, xs, ys); }
    inline QString toText(int indent, const Selection &s, int format, Document *doc)
    {
        Q_UNUSED(s)
        return convertToText(selectAll(), indent + 2, format, doc);
    }
    inline void cloneStyleFrom(Grid *o) {
        bordercolor = o->bordercolor;
        // TODO: what others?
    }
    static int fillRows(Grid *g, const QStringList &as, int column, int startrow, int starty);
};

#endif // GRID_H
