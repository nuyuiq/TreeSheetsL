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
    int colWidth();
    int depth() const;
    Cell *parent(int i);
    void addUndo(Document *doc);
    void resetLayout();
    void reset();
    void resetChildren();
    bool isTag(Document *doc) const;
    // the smallest relsize is actually the biggest text
    int minRelsize();
    bool gridShown(Document *doc) const;
    void lazyLayout(Document *doc, QPainter &dc, int depth, int maxcolwidth, bool forcetiny);
    void layout(Document *doc, QPainter &dc, int depth, int maxcolwidth, bool forcetiny);
    void render(Document *doc, int bx, int by, QPainter &dc, int depth, int ml, int mr, int mt, int mb, int maxcolwidth, int cell_margin);

    inline bool hasText() const { return !text.t.isEmpty(); }
    inline bool hasTextSize() const { return hasText() || text.relsize; }
    inline bool hasTextState() const { return hasTextSize() || text.image.data(); }
    inline bool hasHeader() const { return hasText() || text.image.data(); }
    inline bool hasContent() const { return hasHeader() || grid; }

    static Cell*loadWhich(Tools::DataIO &dis, Cell *parent, int &numcells, int &textbytes, QVariantMap &info);
};



#endif // CELL_H
