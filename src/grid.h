#ifndef GRID_H
#define GRID_H

#include "selection.h"

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
    void initColWidths();
    void setOrient();
    Selection findCell(Cell *o);
    void move(int dx, int dy, Selection &s);
    void drawSelect(Document *doc, QPainter &dc, Selection &s, bool cursoronly);
    void insertCells(int dx, int dy, int nxs, int nys, Cell *nc = nullptr);
    int minRelsize(int rs);
    void resetChildren();
    bool layout(Document *doc, QPainter &dc, int depth, int &sx, int &sy, int startx, int starty, bool forcetiny);
    void render(Document *doc, int bx, int by, QPainter &dc, int depth, int sx, int sy, int xoff, int yoff);
};

#endif // GRID_H
