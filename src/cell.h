#ifndef CELL_H
#define CELL_H

#include "text.h"

#include <QVariantMap>


struct Document;
struct Grid;
namespace Tools { class DataIO; }


/* The evaluation types for a cell.
CT_DATA: "Data"
CT_CODE: "Operation"
CT_VARD: "Variable Assign"
CT_VARU: "Variable Read"
CT_VIEWH: "Horizontal View"
CT_VIEWV: "Vertical View"
*/
enum CellType{
    CT_DATA = 0,
    CT_CODE,
    CT_VARD,
    CT_VIEWH,
    CT_VARU,
    CT_VIEWV};

/* The drawstyles for a cell:

*/
enum {
    DS_GRID,
    DS_BLOBSHIER,
    DS_BLOBLINE };

struct Cell
{
    Text text;
    Cell *p;
    Grid *grid;
    quint32 cellcolor;
    quint32 textcolor;
    int sx;
    int sy;
    int ox;
    int oy;
    int minx;
    int miny;
    int ycenteroff;
    int txs;
    int tys;
    uint actualcellcolor;
    quint8 celltype;
    quint8 drawstyle;
    bool verticaltextandgrid;
    bool tiny;


    Cell(Cell *_p = nullptr,
         const Cell *_clonefrom = nullptr,
         quint8 _ct = CT_DATA,
         Grid *_g = nullptr);
    ~Cell();
    void cloneStyleFrom(Cell const *o);
    bool loadGrid(Tools::DataIO &dis, int &numcells, int &textbytes, QVariantMap &info);
    void save(Tools::DataIO &dos, const QVector<ImagePtr> &imgs) const;
    int colWidth();
    int depth() const;
    Cell *parent(int i);
    void addUndo(Document *doc);
    void resetLayout();
    void reset();
    void resetChildren();
    bool isTag(Document *doc) const;
    int getX(Document *doc) const;
    int getY(Document *doc) const;
    // the smallest relsize is actually the biggest text
    int minRelsize();
    bool gridShown(Document *doc) const;
    void lazyLayout(Document *doc, QPainter &dc, int depth, int maxcolwidth, bool forcetiny);
    void layout(Document *doc, QPainter &dc, int depth, int maxcolwidth, bool forcetiny);
    void render(Document *doc, int bx, int by, QPainter &dc, int depth, int ml, int mr, int mt, int mb, int maxcolwidth, int cell_margin);
    void relSize(int dir, int zoomdepth);
    bool isParentOf(const Cell *c) const;
    void clear();
    void paste(Document *doc, const Cell *c, Selection &s);
    Cell* clone(Cell *_parent) const;
    void imageRefCollect(QVector<ImagePtr> &imgs);
    void maxDepthLeaves(int curdepth, int &maxdepth, int &leaves);
    QString toText(int indent, const Selection &s, int format, Document *doc);
    void collectCells(QVector<Cell *> &itercells, bool recurse = true);
    Cell *findNextSearchMatch(const QString &search, Cell *best, Cell *selected, bool &lastwasselected);
    void findReplaceAll(const QString &str);
    Grid *addGrid(int x = 1, int y = 1);
    void setBorder(int width);
    void setGridTextLayout(int ds, bool vert, bool noset);
    Cell *findLink(Selection &s, Cell *link, Cell *best, bool &lastthis, bool &stylematch,
                    bool forward);
    Cell *findExact(const QString &s);
    void colorChange(int which, uint color);

    inline bool hasText() const { return !text.t.isEmpty(); }
    inline bool hasTextSize() const { return hasText() || text.relsize; }
    inline bool hasTextState() const { return hasTextSize() || text.image.data(); }
    inline bool hasHeader() const { return hasText() || text.image.data(); }
    inline bool hasContent() const { return hasHeader() || grid; }
    inline bool isInside(int x, int y) const { return x >= 0 && y >= 0 && x < sx && y < sy; }


    static Cell*loadWhich(Tools::DataIO &dis, Cell *parent, int &numcells, int &textbytes, QVariantMap &info);
};



#endif // CELL_H
