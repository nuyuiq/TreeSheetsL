#include "cell.h"
#include "grid.h"
#include "tools.h"
#include "config.h"
#include "myapp.h"
#include "document.h"

#include <QVariantMap>

Cell::Cell(Cell *_p, const Cell *_clonefrom, quint8 _ct, Grid *_g)
{
    p = _p;
    grid = _g;
    celltype = _ct;
    verticaltextandgrid = 0;
    cellcolor = 0xFFFFFF;
    textcolor = 0x000000;
    drawstyle = DS_GRID;
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
