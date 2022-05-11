#include "grid.h"
#include "cell.h"
#include "config.h"
#include "myapp.h"
#include "selection.h"
#include "tools.h"

#define foreachcell(c)                \
    for (int y = 0; y < ys; y++)      \
        for (int x = 0; x < xs; x++)  \
            for (bool _f = true; _f;) \
                for (Cell *&c = C(x, y); _f; _f = false)
#define foreachcellrev(c)                 \
    for (int y = ys - 1; y >= 0; y--)     \
        for (int x = xs - 1; x >= 0; x--) \
            for (bool _f = true; _f;)     \
                for (Cell *&c = C(x, y); _f; _f = false)
#define foreachcelly(c)           \
    for (int y = 0; y < ys; y++)  \
        for (bool _f = true; _f;) \
            for (Cell *&c = C(0, y); _f; _f = false)
#define foreachcellcolumn(c)          \
    for (int x = 0; x < xs; x++)      \
        for (int y = 0; y < ys; y++)  \
            for (bool _f = true; _f;) \
                for (Cell *&c = C(x, y); _f; _f = false)
#define foreachcellinsel(c, s)                 \
    for (int y = s.y; y < s.y + s.ys; y++)     \
        for (int x = s.x; x < s.x + s.xs; x++) \
            for (bool _f = true; _f;)          \
                for (Cell *&c = C(x, y); _f; _f = false)
#define foreachcellinselrev(c, s)                   \
    for (int y = s.y + s.ys - 1; y >= s.y; y--)     \
        for (int x = s.x + s.xs - 1; x >= s.x; x--) \
            for (bool _f = true; _f;)               \
                for (Cell *&c = C(x, y); _f; _f = false)
#define foreachcellingrid(c, g)         \
    for (int y = 0; y < g->ys; y++)     \
        for (int x = 0; x < g->xs; x++) \
            for (bool _f = true; _f;)   \
                for (Cell *&c = g->C(x, y); _f; _f = false)

Grid::Grid(int _xs, int _ys, Cell *_c)
{
    this->cell = _c;
    this->xs = _xs;
    this->ys = _ys;
    this->cells = new Cell *[_xs * _ys];
    this->colwidths = nullptr;
    this->user_grid_outer_spacing = 3;
    this->bordercolor = 0xA0A0A0;
    this->horiz = false;
    this->folded = false;

    foreachcell(c) c = nullptr;
    initColWidths();
    setOrient();
}

Grid::~Grid()
{
    foreachcell(c) if (c) delete c;
    delete[] cells;
    if (colwidths) delete[] colwidths;
}

bool Grid::loadContents(Tools::DataIO &dis, int &numcells, int &textbytes, QVariantMap &info)
{
    int version = info.value(QStringLiteral("version")).toInt();
    if (version >= 10) {
        bordercolor = dis.readInt32() & 0xFFFFFF;
        user_grid_outer_spacing = dis.readInt32();
        if (version >= 11) {
            cell->verticaltextandgrid = dis.readInt8() != 0;
            if (version >= 13) {
                if (version >= 16) {
                    folded = dis.readInt8() != 0;
                    if (folded && version <= 17) {
                        // Before v18, folding would use the image slot. So if this cell
                        // contains an image, clear it.
                        cell->text.image.clear();
                    }
                }
                for (int x = 0; x < xs; x++)
                {
                    colwidths[x] = dis.readInt32();
                }
            }
        }
    }
    foreachcell(c)
    {
        c = Cell::loadWhich(dis, cell, numcells, textbytes, info);
        if (!c) return false;
    }
    return true;
}

void Grid::initColWidths()
{
    // TODO
    if (colwidths) delete[] colwidths;
    colwidths = new int[xs];
    int cw = cell ? cell->colWidth() : myApp.cfg->defaultmaxcolwidth;
    for (int x = 0; x < xs; x++)
    {
        colwidths[x] = cw;
    }
}

void Grid::setOrient()
{
    if (xs > ys) horiz = true;
    else if (ys > xs) horiz = false;
}

Selection Grid::findCell(Cell *o)
{
    foreachcell(c)
    {
        if (c == o)
        {
            return Selection(this, x, y, 1, 1);
        }
    }
    return Selection();
}

void Grid::move(int dx, int dy, Selection &s)
{
    if (dx < 0 || dy < 0)
        foreachcellinsel(c, s) qSwap(c, C((x + dx + xs) % xs, (y + dy + ys) % ys));
    else
        foreachcellinselrev(c, s) qSwap(c, C((x + dx + xs) % xs, (y + dy + ys) % ys));
}

void Grid::drawSelect(Document *doc, QPainter &dc, Selection &s, bool cursoronly)
{
//#ifndef SIMPLERENDER
//    dc.SetLogicalFunction(wxINVERT);
//#endif
//    if (s.Thin()) {
//        if (!cursoronly) DrawInsert(doc, dc, s, 0);
//    } else {
//        if (!cursoronly) {
//            dc.SetBrush(*wxBLACK_BRUSH);
//            dc.SetPen(*wxBLACK_PEN);
//            wxRect g = GetRect(doc, s);
//            int lw = g_line_width;
//            int te = s.TextEdit();
//            dc.DrawRectangle(g.x - 1 - lw, g.y - 1 - lw, g.width + 2 + 2 * lw, 2 + lw - te);
//            dc.DrawRectangle(g.x - 1 - lw, g.y - 1 + g.height + te, g.width + 2 + 2 * lw - 5,
//                             2 + lw - te);

//            dc.DrawRectangle(g.x - 1 - lw, g.y + 1 - te, 2 + lw - te, g.height - 2 + 2 * te);
//            dc.DrawRectangle(g.x - 1 + g.width + te, g.y + 1 - te, 2 + lw - te,
//                             g.height - 2 + 2 * te - 2 - te);

//            dc.DrawRectangle(g.x + g.width, g.y + g.height - 2, lw + 2, lw + 4);
//            dc.DrawRectangle(g.x + g.width - lw - 1, g.y + g.height - 2 + 2 * te, lw + 1,
//                             lw + 4 - 2 * te);
//        }
//#ifndef SIMPLERENDER
//        dc.SetLogicalFunction(wxXOR);
//#endif
//        if (s.TextEdit())
//#ifdef SIMPLERENDER
//            DrawCursor(doc, dc, s, true, 0x00FF00, cursoronly);
//#else
//            DrawCursor(doc, dc, s, true, 0xFFFF, cursoronly);
//#endif
//    }
//#ifndef SIMPLERENDER
//    dc.SetLogicalFunction(wxCOPY);
//#endif
}

void Grid::insertCells(int dx, int dy, int nxs, int nys, Cell *nc) {
    Q_ASSERT(((dx < 0) == (nxs == 0)) &&
           ((dy < 0) == (nys == 0)));
    Q_ASSERT(nxs + nys == 1);
    Cell **ocells = cells;
    cells = new Cell *[(xs + nxs) * (ys + nys)];
    int *ocw = colwidths;
    colwidths = new int[xs + nxs];
    xs += nxs;
    ys += nys;
    Cell **ncp = ocells;
    setOrient();
    foreachcell(c)
    {
        if (x == dx || y == dy)
        {
            if (nc) c = nc;
            else
            {
                Cell *colcell = ocells[(nxs ? qMin(dx, xs - nxs - 1) : x) +
                        (nxs ? y : qMin(dy, ys - nys - 1)) * (xs - nxs)];
                c = new Cell(cell, colcell);
                c->text.relsize = colcell->text.relsize;
            }
        }
        else c = *ncp++;
    }
    int *cwp = ocw;
    for (int x = 0; x < xs; x++)
    {
        colwidths[x] = x == dx ? cell->colWidth() : *cwp++;
    }
    delete[] ocells;
    delete[] ocw;
}
