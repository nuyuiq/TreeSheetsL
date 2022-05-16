#include "grid.h"
#include "cell.h"
#include "config.h"
#include "myapp.h"
#include "selection.h"
#include "tools.h"
#include "mainwindow.h"
#include "document.h"
#include "widget.h"

#include <QPainter>
#include <QDebug>

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
#ifndef SIMPLERENDER
    auto old = dc.compositionMode();
    dc.setCompositionMode(QPainter::RasterOp_NotDestination);
#endif
    if (s.thin())
    {
        if (!cursoronly) drawInsert(doc, dc, s, 0);
    }
    else
    {
        if (!cursoronly)
        {
            dc.setBrush(QBrush(Qt::black));
            dc.setPen(QColor(Qt::black));
            const QRect &g = getRect(doc, s);
            const int lw = _g::line_width;
            const int te = s.textEdit();
            Tools::drawRect(dc,
                            g.x() - 1 - lw,
                            g.y() - 1 - lw,
                            g.width() + 2 + 2 * lw,
                            2 + lw - te);
            Tools::drawRect(dc,
                            g.x() - 1 - lw,
                            g.y() - 1 + g.height() + te,
                            g.width() + 2 + 2 * lw - 5,
                            2 + lw - te);

            Tools::drawRect(dc,
                            g.x() - 1 - lw,
                            g.y() + 1 - te,
                            2 + lw - te,
                            g.height() - 2 + 2 * te);
            Tools::drawRect(dc,
                            g.x() - 1 + g.width() + te,
                            g.y() + 1 - te,
                            2 + lw - te,
                            g.height() - 2 + 2 * te - 2 - te);

            Tools::drawRect(dc,
                            g.x() + g.width(),
                            g.y() + g.height() - 2,
                            lw + 2,
                            lw + 4);
            Tools::drawRect(dc,
                            g.x() + g.width() - lw - 1,
                            g.y() + g.height() - 2 + 2 * te,
                            lw + 1,
                            lw + 4 - 2 * te);
        }
#ifndef SIMPLERENDER
        dc.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
#endif
        if (s.textEdit())
        {
#ifdef SIMPLERENDER
            drawCursor(doc, dc, s, 0x00FF00, cursoronly);
#else
            drawCursor(doc, dc, s, 0xFFFF, cursoronly);
#endif
        }
    }
#ifndef SIMPLERENDER
    dc.setCompositionMode(old);
#endif
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

int Grid::minRelsize(int rs)
{
    foreachcell(c)
    {
        int crs = c->minRelsize();
        rs = qMin(rs, crs);
    }
    return rs;
}

void Grid::resetChildren()
{
    cell->reset();
    foreachcell(c)
    {
        c->resetChildren();
    }
}

bool Grid::layout(Document *doc, QPainter &dc, int depth, int &sx, int &sy, int startx, int starty, bool forcetiny)
{
    int *xa = new int[xs];
    int *ya = new int[ys];
    for (int i = 0; i < xs; i++) xa[i] = 0;
    for (int i = 0; i < ys; i++) ya[i] = 0;
    tinyborder = true;
    foreachcell(c)
    {
        c->lazyLayout(doc, dc, depth + 1, colwidths[x], forcetiny);
        tinyborder = c->tiny && tinyborder;
        xa[x] = qMax(xa[x], c->sx);
        ya[y] = qMax(ya[y], c->sy);
    }
    view_grid_outer_spacing =
            tinyborder || cell->drawstyle != DS_GRID ? 0 : user_grid_outer_spacing;
    view_margin = tinyborder || cell->drawstyle != DS_GRID ? 0 : _g::grid_margin;
    cell_margin = tinyborder ? 0 : (cell->drawstyle == DS_GRID ? 0 : _g::cell_margin);

    sx = (xs + 1) * _g::line_width + xs * cell_margin * 2 +
            2 * (view_grid_outer_spacing + view_margin) + startx;
    sy = (ys + 1) * _g::line_width + ys * cell_margin * 2 +
            2 * (view_grid_outer_spacing + view_margin) + starty;
    for (int i = 0; i < xs; i++) sx += xa[i];
    for (int i = 0; i < ys; i++) sy += ya[i];
    int cx = view_grid_outer_spacing + view_margin + _g::line_width + cell_margin + startx;
    int cy = view_grid_outer_spacing + view_margin + _g::line_width + cell_margin + starty;
    if (!cell->tiny)
    {
        cx += _g::margin_extra;
        cy += _g::margin_extra;
    }
    foreachcell(c)
    {
        c->ox = cx;
        c->oy = cy;
        if (c->drawstyle == DS_BLOBLINE && !c->grid)
        {
            Q_ASSERT(c->sy <= ya[y]);
            c->ycenteroff = (ya[y] - c->sy) / 2;
        }
        c->sx = xa[x];
        c->sy = ya[y];
        cx += xa[x] + _g::line_width + cell_margin * 2;
        if (x == xs - 1)
        {
            cy += ya[y] + _g::line_width + cell_margin * 2;
            cx = view_grid_outer_spacing + view_margin + _g::line_width + cell_margin + startx;
            if (!cell->tiny) cx += _g::margin_extra;
        }
    }
    delete[] xa;
    delete[] ya;
    return tinyborder;
}

void Grid::render(Document *doc, int bx, int by, QPainter &dc, int depth)
{
    int xoff = C(0, 0)->ox - view_margin - view_grid_outer_spacing - 1;
    int yoff = C(0, 0)->oy - view_margin - view_grid_outer_spacing - 1;
    int maxx = C(xs - 1, 0)->ox + C(xs - 1, 0)->sx;
    int maxy = C(0, ys - 1)->oy + C(0, ys - 1)->sy;
    if (tinyborder || cell->drawstyle == DS_GRID)
    {
        int ldelta = view_grid_outer_spacing != 0;
        auto drawlines = [&]{
            for (int x = ldelta; x <= xs - ldelta; x++)
            {
                int xl = (x == xs ? maxx : C(x, 0)->ox - _g::line_width) + bx;
                if (xl >= doc->originx && xl <= doc->maxx)
                {
                    for (int line = 0; line < _g::line_width; line++)
                    {
                        Tools::drawLine(dc,
                                    xl + line, qMax(doc->originy, by + yoff + view_grid_outer_spacing),
                                    xl + line, qMin(doc->maxy, by + maxy + _g::line_width) + view_margin);
                    }
                }
            }
            for (int y = ldelta; y <= ys - ldelta; y++)
            {
                int yl = (y == ys ? maxy : C(0, y)->oy - _g::line_width) + by;
                if (yl >= doc->originy && yl <= doc->maxy)
                {
                    for (int line = 0; line < _g::line_width; line++)
                    {
                        Tools::drawLine(dc, qMax(doc->originx,
                                        bx + xoff + view_grid_outer_spacing + _g::line_width),
                                    yl + line, qMin(doc->maxx, bx + maxx) + view_margin,
                                    yl + line);
                    }
                }
            }
        };
        if (!myApp.cfg->fastrender && view_grid_outer_spacing && cell->cellcolor != 0xFFFFFF)
        {
            dc.setPen(QColor(Qt::white));
            drawlines();
        }
        // dotted lines result in very expensive drawline calls
        dc.setPen(Color(view_grid_outer_spacing && !myApp.cfg->fastrender ?
                      myApp.cfg->pen_gridlines : myApp.cfg->pen_tinygridlines));
        drawlines();
    }

    foreachcell(c)
    {
        int cx = bx + c->ox;
        int cy = by + c->oy;
        if (cx < doc->maxx && cx + c->sx > doc->originx && cy < doc->maxy &&
                cy + c->sy > doc->originy)
        {
            c->render(doc, cx, cy, dc, depth + 1, (x == 0) * view_margin,
                      (x == xs - 1) * view_margin, (y == 0) * view_margin,
                      (y == ys - 1) * view_margin, colwidths[x], cell_margin);
        }
    }

    if (cell->drawstyle == DS_BLOBLINE && !tinyborder && cell->hasHeader() && !cell->tiny)
    {
        const int arcsize = 8;
        int srcy = by + cell->ycenteroff +
                (cell->verticaltextandgrid ? cell->tys + 2 : cell->tys / 2) + _g::margin_extra;
        // fixme: the 8 is chosen to fit the smallest text size, not very portable
        int srcx = bx + (cell->verticaltextandgrid ? 8 : cell->txs + 4) + _g::margin_extra;
        int destyfirst = -1, destylast = -1;
        dc.setPen(QColor(Qt::gray));
        foreachcelly(c) if (c->hasContent() && !c->tiny)
        {
            int desty = c->ycenteroff + by + c->oy + c->tys / 2 + _g::margin_extra;
            int destx = bx + c->ox - 2 + _g::margin_extra;
            bool visible = srcx < doc->maxx && destx > doc->originx &&
                    desty - arcsize < doc->maxy && desty + arcsize > doc->originy;
            if (abs(srcy - desty) < arcsize && !cell->verticaltextandgrid)
            {
                if (destyfirst < 0) destyfirst = desty;
                destylast = desty;
                if (visible) Tools::drawLine(dc, srcx, desty, destx, desty);
            }
            else
            {
                if (desty < srcy)
                {
                    if (destyfirst < 0) destyfirst = desty + arcsize;
                    destylast = desty + arcsize;
                    if (visible && !!myApp.frame->line_nw)
                    {
                        Tools::drawImage(dc, srcx, desty, myApp.frame->line_nw->display());
                    }
                }
                else
                {
                    destylast = desty - arcsize;
                    if (visible && !!myApp.frame->line_sw)
                    {
                        Tools::drawImage(dc, srcx, desty - arcsize, myApp.frame->line_sw->display());
                    }
                    desty--;
                }
                if (visible) Tools::drawLine(dc, srcx + arcsize, desty, destx, desty);
            }
        }
        if (cell->verticaltextandgrid)
        {
            if (destylast > 0) Tools::drawLine(dc, srcx, srcy, srcx, destylast);
        }
        else
        {
            if (destyfirst >= 0 && destylast >= 0 && destyfirst < destylast)
            {
                destyfirst = qMin(destyfirst, srcy);
                destylast = qMax(destylast, srcy);
                Tools::drawLine(dc, srcx, destyfirst, srcx, destylast);
            }
        }
    }
    if (view_grid_outer_spacing && cell->drawstyle == DS_GRID)
    {
        dc.setBrush(QColor(Qt::transparent));
        dc.setPen(Color(bordercolor));
        for (int i = 0; i < view_grid_outer_spacing - 1; i++)
        {
            QRect rect(bx + xoff + view_grid_outer_spacing - i,
                        by + yoff + view_grid_outer_spacing - i,
                        maxx - xoff - view_grid_outer_spacing + 1 + i * 2 + view_margin,
                        maxy - yoff - view_grid_outer_spacing + 1 + i * 2 + view_margin);
            Tools::drawRoundedRect(dc, myApp.cfg->roundness, rect);
        }
    }
}

void Grid::drawHover(Document *doc, QPainter &dc, Selection &s)
{
#ifndef SIMPLERENDER
#ifdef Q_OS_MAC
    const uint thincol = 0xFFFFFF;
    const uint bgcol = 0xFFFFFF;
#else
    const uint thincol = 0x555555;
    const uint bgcol = 0x101014;
#endif
    auto old = dc.compositionMode();
    dc.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
    if (s.thin())
    {
        drawInsert(doc, dc, s, thincol);
    }
    else
    {
        const Cell *c = C(s.x, s.y);
        QRect rect = QRect(c->getX(doc) - cell_margin,
                                c->getY(doc) - cell_margin,
                                c->sx + cell_margin * 2,
                                c->sy + cell_margin * 2);
        Tools::drawRect(dc, bgcol, rect.x(), rect.y(), rect.width(), rect.height());
    }
    dc.setCompositionMode(old);
#endif
}

void Grid::drawInsert(Document *doc, QPainter &dc, Selection &s, uint colour)
{
    QVector<QRect> lines(_g::line_width);
    QRect bline;
    if (!s.xs)
    {
        Cell *c = C(s.x - (s.x == xs), s.y);
        int x = c->getX(doc) + (c->sx + _g::line_width + cell_margin) * (s.x == xs) -
                _g::line_width - cell_margin;
        int y1 = qMax(cell->getY(doc), doc->originy);
        int y2 = qMin(cell->getY(doc) + cell->sy, doc->maxy);
        for (int line = 0; line < _g::line_width; line++)
        {
            lines[line].setCoords(x + line, y1, x + line, y2);
        }
        bline.setRect(x - 1, c->getY(doc), _g::line_width + 2, c->sy);

    }
    else
    {
        Cell *c = C(s.x, s.y - (s.y == ys));
        int y = c->getY(doc) + (c->sy + _g::line_width + cell_margin) * (s.y == ys) -
                _g::line_width - cell_margin;
        int x1 = qMax(cell->getX(doc), doc->originx);
        int x2 = qMin(cell->getX(doc) + cell->sx, doc->maxx);
        for (int line = 0; line < _g::line_width; line++)
        {
            lines[line].setCoords(x1, y + line, x2, y + line);
        }
        bline.setRect(c->getX(doc), y - 1, c->sx, _g::line_width + 2);
    }

    QPen pen(dc.pen());
    pen.setColor(Color(myApp.cfg->pen_thinselect));
    pen.setStyle(Qt::CustomDashLine);
    pen.setDashPattern(QVector<qreal>() << 2 << 4);
    dc.setPen(pen);
    for (auto itr = lines.constBegin(); itr != lines.constEnd(); itr++)
    {
        Tools::drawLine(dc, itr->left(), itr->top(), itr->right(), itr->bottom());
    }
    Tools::drawRect(dc, colour, bline.x(), bline.y(), bline.width(), bline.height());
}

void Grid::findXY(Document *doc, int px, int py, QPainter &dc)
{
    foreachcell(c)
    {
        int bx = px - c->ox;
        int by = py - c->oy;
        if (bx >= 0 && by >= -_g::line_width - _g::selmargin && bx < c->sx && by < _g::selmargin)
        {
            doc->hover = Selection(this, x, y, 1, 0);
            return;
        }
        if (bx >= 0 && by >= c->sy - _g::selmargin && bx < c->sx &&
                by < c->sy + _g::line_width + _g::selmargin)
        {
            doc->hover = Selection(this, x, y + 1, 1, 0);
            return;
        }
        if (bx >= -_g::line_width - _g::selmargin && by >= 0 && bx < _g::selmargin && by < c->sy)
        {
            doc->hover = Selection(this, x, y, 0, 1);
            return;
        }
        if (bx >= c->sx - _g::selmargin && by >= 0 && bx < c->sx + _g::line_width + _g::selmargin &&
                by < c->sy)
        {
            doc->hover = Selection(this, x + 1, y, 0, 1);
            return;
        }
        if (c->isInside(bx, by))
        {
            if (c->gridShown(doc)) c->grid->findXY(doc, bx, by, dc);
            if (doc->hover.g) return;
            doc->hover = Selection(this, x, y, 1, 1);
            if (c->hasText())
            {
                c->text.findCursor(doc, bx, by - c->ycenteroff, dc, doc->hover, colwidths[x]);
            }
            return;
        }
    }
}

void Grid::resizeColWidths(int dir, const Selection &s, bool hierarchical)
{
    for (int x = s.x; x < s.x + s.xs; x++)
    {
        colwidths[x] += dir * 5;
        if (colwidths[x] < 5) colwidths[x] = 5;
        for (int y = 0; y < ys; y++)
        {
            Cell *c = C(x, y);
            if (c->grid && hierarchical)
            {
                c->grid->resizeColWidths(dir, c->grid->selectAll(), hierarchical);
            }
        }
    }
}

void Grid::relSize(int dir, int zoomdepth)
{
    foreachcell(c) c->relSize(dir, zoomdepth);
}

void Grid::relSize(int dir, Selection &s, int zoomdepth)
{
    foreachcellinsel(c, s) c->relSize(dir, zoomdepth);
}

void Grid::multiCellDeleteSub(Document *doc, Selection &s)
{
    foreachcellinsel(c, s) c->clear();
    bool delhoriz = true, delvert = true;
    foreachcell(c)
    {
        if (c->hasContent())
        {
            if (y >= s.y && y < s.y + s.ys) delhoriz = false;
            if (x >= s.x && x < s.x + s.xs) delvert = false;
        }
    }
    if (delhoriz && (!delvert || s.xs >= s.ys))
    {
        if (s.ys == ys)
        {
            delSelf(doc, s);
        }
        else
        {
            for (int i = 0; i < s.ys; i++) deleteCells(-1, s.y, 0, -1);
            s.ys = 0;
            s.xs = 1;
        }
    }
    else if (delvert)
    {
        if (s.xs == xs)
        {
            delSelf(doc, s);
        }
        else
        {
            for (int i = 0; i < s.xs; i++) deleteCells(s.x, -1, -1, 0);
            s.xs = 0;
            s.ys = 1;
        }
    }
    else
    {
        Cell *c = s.getCell();
        if (c) s.enterEdit(doc);
    }
}

void Grid::delSelf(Document *doc, Selection &s)
{
    for (auto c = doc->curdrawroot; c; c = c->p)
        if (c == cell) return;  // FIXME, if drawroot, auto zoom out instead (needs dc)
    s = cell->p->grid->findCell(cell);
    Grid *&pthis = cell->grid;
    DELPTR(pthis);
}

void Grid::deleteCells(int dx, int dy, int nxs, int nys)
{
    Cell **ncells = new Cell *[(xs + nxs) * (ys + nys)];
    Cell **ncp = ncells;
    int *ncw = new int[xs + nxs];
    int *ncwp = ncw;
    foreachcell(c) if (x == dx || y == dy)
    {
        DELPTR(c);
    }
    else *ncp++ = c;
    for (int x = 0; x < xs; x++) if (x != dx) *ncwp++ = colwidths[x];
    delete[] cells;
    cells = ncells;
    delete[] colwidths;
    colwidths = ncw;
    xs += nxs;
    ys += nys;
    setOrient();
}

void Grid::clone(Grid *g)
{
    g->bordercolor = bordercolor;
    g->user_grid_outer_spacing = user_grid_outer_spacing;
    g->folded = folded;
    foreachcell(c) g->C(x, y) = c->clone(g->cell);
    for (int x = 0; x < xs; x++) g->colwidths[x] = colwidths[x];
}

void Grid::mergeWithParent(Grid *p, Selection &s)
{
    cell->grid = nullptr;
    foreachcell(c)
    {
        if (x + s.x >= p->xs) p->insertCells(p->xs, -1, 1, 0);
        if (y + s.y >= p->ys) p->insertCells(-1, p->ys, 0, 1);
        Cell *pc = p->C(x + s.x, y + s.y);
        if (pc->hasContent())
        {
            if (x) p->insertCells(s.x + x, -1, 1, 0);
            pc = p->C(x + s.x, y + s.y);
            if (pc->hasContent()) {
                if (y) p->insertCells(-1, s.y + y, 0, 1);
                pc = p->C(x + s.x, y + s.y);
            }
        }
        delete pc;
        p->C(x + s.x, y + s.y) = c;
        c->p = p->cell;
        c = nullptr;
    }
    s.g = p;
    s.xs += xs - 1;
    s.ys += ys - 1;
    delete this;
}

QRect Grid::getRect(Document *doc, Selection &s, bool minimal)
{
    if (s.thin())
    {
        if (s.xs)
        {
            if (s.y < ys)
            {
                Cell *tl = C(s.x, s.y);
                return QRect(tl->getX(doc), tl->getY(doc), tl->sx, 0);
            }
            else
            {
                Cell *br = C(s.x, ys - 1);
                return QRect(br->getX(doc), br->getY(doc) + br->sy, br->sx, 0);
            }
        }
        else
        {
            if (s.x < xs)
            {
                Cell *tl = C(s.x, s.y);
                return QRect(tl->getX(doc), tl->getY(doc), 0, tl->sy);
            }
            else
            {
                Cell *br = C(xs - 1, s.y);
                return QRect(br->getX(doc) + br->sx, br->getY(doc), 0, br->sy);
            }
        }
    }
    else
    {
        Cell *tl = C(s.x, s.y);
        Cell *br = C(s.x + s.xs - 1, s.y + s.ys - 1);
        QRect r(tl->getX(doc) - cell_margin, tl->getY(doc) - cell_margin,
                 br->getX(doc) + br->sx - tl->getX(doc) + cell_margin * 2,
                 br->getY(doc) + br->sy - tl->getY(doc) + cell_margin * 2);
        if (minimal && tl == br) r.setWidth(qMax(r.width() - (tl->sx - tl->minx), 0));
        return r;
    }
}

void Grid::drawCursor(Document *doc, QPainter &dc, Selection &s, uint color, bool cursoronly)
{
    Cell *c = s.getCell();
    if (c && !c->tiny && (c->hasText() || !c->grid))
    {
        c->text.drawCursor(doc, dc, s, color, cursoronly, colwidths[s.x]);
    }
}
