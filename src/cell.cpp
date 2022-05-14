#include "cell.h"
#include "grid.h"
#include "tools.h"
#include "config.h"
#include "myapp.h"
#include "document.h"

#include <QPainter>
#include <QDebug>

Cell::Cell(Cell *_p, const Cell *_clonefrom, quint8 _ct, Grid *_g)
{
    p = _p;
    grid = _g;
    celltype = _ct;
    verticaltextandgrid = 0;
    cellcolor = 0xFFFFFF;
    textcolor = 0x000000;
    drawstyle = DS_GRID;
    tiny = false;
    actualcellcolor = 0xFFFFFF;
    reset();

    text.cell = this;
    if (_g) _g->cell = this;
    if (_p)
    {
        text.relsize = _p->text.relsize;
        verticaltextandgrid = _p->verticaltextandgrid;
    }
    if (_clonefrom)
    {
        cloneStyleFrom(_clonefrom);
    }
}

Cell::~Cell()
{
    DELPTR(grid);
}

void Cell::cloneStyleFrom(const Cell *o)
{
    cellcolor = o->cellcolor;
    textcolor = o->textcolor;
    verticaltextandgrid = o->verticaltextandgrid;
    drawstyle = o->drawstyle;
    text.stylebits = o->text.stylebits;
}

bool Cell::loadGrid(Tools::DataIO &dis, int &numcells, int &textbytes, QVariantMap &info)
{
    int xs = dis.readInt32();
    grid = new Grid(xs, dis.readInt32(), this);
    bool ret = grid->loadContents(dis, numcells, textbytes, info);
    if (!ret) DELPTR(grid);
    return ret;
}

int Cell::colWidth()
{
    return p ? p->grid->colwidths[p->grid->findCell(this).x]
            : myApp.cfg->defaultmaxcolwidth;
}

Cell *Cell::loadWhich(Tools::DataIO &dis, Cell *parent, int &numcells, int &textbytes, QVariantMap &info)
{
    Cell *c = new Cell(parent, nullptr, dis.readInt8());
    numcells++;
    int version = info.value(QStringLiteral("version")).toInt();
    if (version >= 8)
    {
        c->cellcolor = dis.readInt32() & 0xFFFFFF;
        c->textcolor = dis.readInt32() & 0xFFFFFF;
    }
    if (version >= 15) c->drawstyle = dis.readInt8();
    int ts = dis.readInt8();
    switch (ts) {
    case TS_BOTH:
    case TS_TEXT:
        c->text.load(dis, info);
        textbytes += c->text.t.length();
        if (ts == TS_TEXT) return c;
    case TS_GRID:
        if (c->loadGrid(dis, numcells, textbytes, info))
        {
            return c;
        }
        break;
    case TS_NEITHER: return c;
    default: break;
    }
    delete c;
    return nullptr;
}

int Cell::depth() const
{
    return p ? p->depth() + 1 : 0;
}

Cell *Cell::parent(int i)
{
    return i? p->parent(i - 1) : this;
}

void Cell::addUndo(Document *doc)
{
    resetLayout();
    doc->addUndo(this);
}

void Cell::resetLayout()
{
    reset();
    if (p) p->resetLayout();
}

void Cell::reset()
{
    ox = oy = sx = sy = minx = miny = ycenteroff = 0;
}

void Cell::resetChildren()
{
    reset();
    if (grid) grid->resetChildren();
}

bool Cell::isTag(Document *doc) const
{
    return doc->tags.contains(text.t);
}

int Cell::minRelsize()
{
    int rs = INT_MAX;
    if (grid)
    {
        rs = grid->minRelsize(rs);
    }
    else if (hasText())
    {
        // the "else" causes oversized titles but a readable grid when you zoom, if only
        // the grid has been shrunk
        rs = text.minRelsize(rs);
    }
    return rs;
}

bool Cell::gridShown(Document *doc) const
{
    return grid && (!grid->folded || this == doc->curdrawroot);
}

void Cell::lazyLayout(Document *doc, QPainter &dc, int depth, int maxcolwidth, bool forcetiny)
{
    if (sx == 0)
    {
        layout(doc, dc, depth, maxcolwidth, forcetiny);
        minx = sx;
        miny = sy;
    }
    else
    {
        sx = minx;
        sy = miny;
    }
}

void Cell::layout(Document *doc, QPainter &dc, int depth, int maxcolwidth, bool forcetiny)
{
    tiny = (text.filtered && !grid) || forcetiny ||
            doc->pickFont(dc, depth, text.relsize, text.stylebits);

    int charHeight = dc.fontMetrics().height();

    int ixs = 0, iys = 0;
    if (!tiny) text.disImgSize(ixs, iys);
    int leftoffset = 0;
    if (!hasText())
    {
        if (!ixs || !iys) sx = sy = tiny ? 1 : charHeight;
        else leftoffset = charHeight;
    }
    else
    {
        text.textSize(dc, sx, sy, tiny, leftoffset, maxcolwidth);
    }
    if (ixs && iys)
    {
        sx += ixs + 2;
        sy = qMax(iys + 2, sy);
    }
    text.extent = sx + depth * charHeight;
    txs = sx;
    tys = sy;
    if (gridShown(doc))
    {
        if (hasHeader())
        {
            if (verticaltextandgrid)
            {
                int osx = sx;
                if (drawstyle == DS_BLOBLINE && !tiny) sy += 4;
                grid->layout(doc, dc, depth, sx, sy, leftoffset, sy, tiny || forcetiny);
                sx = qMax(sx, osx);
            }
            else
            {
                int osy = sy;
                if (drawstyle == DS_BLOBLINE && !tiny) sx += 18;
                grid->layout(doc, dc, depth, sx, sy, sx, 0, tiny || forcetiny);
                sy = qMax(sy, osy);
            }
        }
        else tiny = grid->layout(doc, dc, depth, sx, sy, 0, 0, forcetiny);
    }
    ycenteroff = !verticaltextandgrid ? (sy - tys) / 2 : 0;
    if (!tiny) {
        sx += _g::margin_extra * 2;
        sy += _g::margin_extra * 2;
    }
}

void Cell::render(Document *doc, int bx, int by, QPainter &dc, int depth, int ml, int mr, int mt, int mb, int maxcolwidth, int cell_margin)
{
    // Choose color from celltype (program operations)
    switch (celltype) {
    case CT_VARD: actualcellcolor = 0xFF8080; break;
    case CT_VARU: actualcellcolor = 0xFFA0A0; break;
    case CT_VIEWH:
    case CT_VIEWV: actualcellcolor = 0x80FF80; break;
    case CT_CODE: actualcellcolor = 0x8080FF; break;
    default: actualcellcolor = cellcolor; break;
    }
    uint parentcolor = doc->background();
    if (p && this != doc->curdrawroot)
    {
        Cell *pp = p;
        while (pp && pp->drawstyle == DS_BLOBLINE)
        {
            pp = pp == doc->curdrawroot ? nullptr : pp->p;
        }
        if (pp) parentcolor = pp->actualcellcolor;
    }
    if (drawstyle == DS_GRID && actualcellcolor != parentcolor)
    {
        Tools::drawRect(dc, actualcellcolor, bx - ml, by - mt, sx + ml + mr, sy + mt + mb);
    }
    if (drawstyle != DS_GRID && hasContent() && !tiny)
    {
        if (actualcellcolor == parentcolor)
        {
            uchar *cp = (uchar *)&actualcellcolor;
            for (int i = 0; i < 4; i++)
            {
                cp[i] = cp[i] * 850 / 1000;
            }
        }
        if (drawstyle == DS_BLOBSHIER)
        {
            Tools::drawRoundedRect(dc, actualcellcolor, myApp.cfg->roundness,
                                   bx - cell_margin, by - cell_margin,
                                   minx + cell_margin * 2, miny + cell_margin * 2);
        }
        else if (hasHeader())
        {
            Tools::drawRoundedRect(dc, actualcellcolor, myApp.cfg->roundness,
                                   bx - cell_margin + _g::margin_extra / 2,
                                   by - cell_margin + ycenteroff + _g::margin_extra / 2,
                                   txs + cell_margin * 2 + _g::margin_extra,
                                   tys + cell_margin * 2 + _g::margin_extra);
        }
        // FIXME: this half a g_margin_extra is a bit of hack
    }

    int xoff = verticaltextandgrid ? 0 : text.extent - depth * dc.fontMetrics().height();
    /*int yoff = */text.render(doc, bx, by + ycenteroff, depth, dc, xoff, maxcolwidth);
    if (gridShown(doc)) grid->render(doc, bx, by, dc, depth);
}

int Cell::getX(Document *doc) const
{
    return ox + (p ? p->getX(doc) : doc->hierarchysize);
}

int Cell::getY(Document *doc) const
{
    return oy + (p ? p->getY(doc) : doc->hierarchysize);
}
