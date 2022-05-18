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

void Cell::save(Tools::DataIO &dos, const QVector<ImagePtr> &imgs) const
{
    dos.writeInt8(celltype);
    dos.writeInt32(cellcolor);
    dos.writeInt32(textcolor);
    dos.writeInt8(drawstyle);
    if (hasTextState())
    {
        dos.writeInt8(grid ? TS_BOTH : TS_TEXT);
        text.save(dos, imgs);
        if (grid) grid->save(dos, imgs);
    }
    else if (grid)
    {
        dos.writeInt8(TS_GRID);
        grid->save(dos, imgs);
    }
    else
    {
        dos.writeInt8(TS_NEITHER);
    }
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
        // 复用 TS_GRID 开关，直落
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

void Cell::relSize(int dir, int zoomdepth)
{
    text.relSize(dir, zoomdepth);
    if (grid) grid->relSize(dir, zoomdepth);
}

bool Cell::isParentOf(const Cell *c) const
{
    return c->p == this || (c->p && isParentOf(c->p));
}

void Cell::clear()
{
    DELPTR(grid);
    text.t.clear();
    text.image = ImagePtr();
    reset();
}

void Cell::paste(Document *doc, const Cell *c, Selection &s)
{
    p->addUndo(doc);
    resetLayout();
    if (c->hasText())
    {
        if (!hasText() || !s.textEdit())
        {
            cellcolor = c->cellcolor;
            textcolor = c->textcolor;
            text.stylebits = c->text.stylebits;
        }
        text.insert(doc, c->text.t, s);
    }
    if (c->text.image) text.image = c->text.image;
    if (c->grid)
    {
        auto cg = new Grid(c->grid->xs, c->grid->ys);
        cg->cell = this;
        c->grid->clone(cg);
        // Note: deleting grid may invalidate c if its a child of grid, so clear it.
        c = nullptr;
        DELPTR(grid);  // FIXME: could merge instead?
        grid = cg;
        if (!hasText()) grid->mergeWithParent(p->grid, s);  // deletes grid/this.
    }
}

Cell *Cell::clone(Cell *_parent) const
{
    Cell* c = new Cell(_parent, this, celltype, grid ? new Grid(grid->xs, grid->ys) : nullptr);
    c->text = text;
    c->text.cell = c;
    if (grid)
    {
        grid->clone(c->grid);
    }
    return c;
}

void Cell::imageRefCollect(QVector<ImagePtr> &imgs)
{
    if (grid) grid->imageRefCollect(imgs);
    if (text.image.data() && !imgs.contains(text.image))
    {
        imgs.append(text.image);
    }
}

void Cell::maxDepthLeaves(int curdepth, int &maxdepth, int &leaves)
{
    if (curdepth > maxdepth) maxdepth = curdepth;
    if (grid) grid->maxDepthLeaves(curdepth + 1, maxdepth, leaves);
    else leaves++;
}

static uint swapColor(uint c)
{
    return ((c & 0xFF) << 16) | (c & 0xFF00) | ((c & 0xFF0000) >> 16);
}

QString Cell::toText(int indent, const Selection &s, int format, Document *doc)
{
    QString str = text.toText(indent, s, format);
    if (format == A_EXPCSV)
    {
        if (grid) return grid->toText(indent, s, format, doc);
        str.replace(QChar::fromLatin1('"'), QStringLiteral("\"\""));
        return QStringLiteral("\"%1\"").arg(str);
    }
    if (s.cursor != s.cursorend) return str;
    const QString sp(indent, QChar::fromLatin1(' '));
    str.append('\n');
    if (grid) str.append(grid->toText(indent, s, format, doc));
    if (format == A_EXPXML)
    {
        str.prepend(">");
        if (text.relsize)
        {
            str.prepend("\"");
            str.prepend(QString::number(-text.relsize));
            str.prepend(" relsize=\"");
        }
        if (text.stylebits)
        {
            str.prepend("\"");
            str.prepend(QString::number(text.stylebits));
            str.prepend(" stylebits=\"");
        }
        if (cellcolor != doc->background())
        {
            str.prepend("\"");
            str.prepend(QString::number(cellcolor));
            str.prepend(" colorbg=\"");
        }
        if (textcolor != 0x000000)
        {
            str.prepend("\"");
            str.prepend(QString::number(textcolor));
            str.prepend(" colorfg=\"");
        }
        if (celltype != CT_DATA)
        {
            str.prepend("\"");
            str.prepend(QString::number(celltype));
            str.prepend(" type=\"");
        }
        str.prepend("<cell");
        str.append(sp);
        str.append("</cell>\n");
    }
    else if (format == A_EXPHTMLT)
    {
        QString style;
        if (text.stylebits & STYLE_BOLD) style += "font-weight: bold;";
        if (text.stylebits & STYLE_ITALIC) style += "font-style: italic;";
        if (text.stylebits & STYLE_FIXED) style += "font-family: monospace;";
        if (text.stylebits & STYLE_UNDERLINE) style += "text-decoration: underline;";
        if (cellcolor != doc->background())
            style += QString().sprintf("background-color: #%06X;", swapColor(cellcolor));
        if (textcolor != 0x000000)
            style += QString().sprintf("color: #%06X;", swapColor(textcolor));
        str.prepend(QStringLiteral("<td style=\"%1\">").arg(style));
        str.append(sp);
        str.append("</td>\n");
    }
    else if (format == A_EXPHTMLB && (text.t.length() || grid) && this != doc->curdrawroot)
    {
        str.prepend("<li>");
        str.append(sp);
        str.append(QString::fromLatin1("</li>\n"));
    } else if (format == A_EXPHTMLO && text.t.length())
    {
        QString h = QString::fromLatin1("h") + QChar::fromLatin1('0' + (indent / 2)) + QString::fromLatin1(">");
        str.prepend(QString::fromLatin1("<") + h);
        str.append(sp);
        str.append(QString::fromLatin1("</") + h + QString::fromLatin1("\n"));
    }
    // TODO
    str.prepend(sp);
    return str;
}

void Cell::collectCells(QVector<Cell *> &itercells, bool recurse)
{
    itercells.append(this);
    if (grid && recurse) grid->collectCells(itercells);
}

Cell *Cell::findNextSearchMatch(const QString &search, Cell *best, Cell *selected, bool &lastwasselected)
{
    if (text.t.toLower().indexOf(search) >= 0)
    {
        if (lastwasselected) best = this;
        lastwasselected = false;
    }
    if (selected == this) lastwasselected = true;
    if (grid) best = grid->findNextSearchMatch(search, best, selected, lastwasselected);
    return best;
}

void Cell::findReplaceAll(const QString &str)
{
    if (grid) grid->findReplaceAll(str);
    text.replaceStr(str);
}

Grid *Cell::addGrid(int x, int y)
{
    if (!grid)
    {
        grid = new Grid(x, y, this);
        grid->initCells(this);
        if (p) grid->cloneStyleFrom(p->grid);
    }
    return grid;
}

void Cell::setBorder(int width)
{
    if (grid) grid->user_grid_outer_spacing = width;
}

void Cell::setGridTextLayout(int ds, bool vert, bool noset)
{
    if (!noset) verticaltextandgrid = vert;
    if (ds != -1) drawstyle = ds;
    if (grid) grid->setGridTextLayout(ds, vert, noset, grid->selectAll());
}

Cell *Cell::findLink(Selection &s, Cell *link, Cell *best, bool &lastthis, bool &stylematch, bool forward)
{
    if (grid) best = grid->findLink(s, link, best, lastthis, stylematch, forward);
    if (link == this)
    {
        lastthis = true;
        return best;
    }
    if (link->text.toText(0, s, A_EXPTEXT) == text.t)
    {
        if (link->text.stylebits != text.stylebits || link->cellcolor != cellcolor ||
                link->textcolor != textcolor)
        {
            if (!stylematch) best = nullptr;
            stylematch = true;
        }
        else if (stylematch)
        {
            return best;
        }
        if (!best || lastthis)
        {
            lastthis = false;
            return this;
        }
    }
    return best;
}

Cell *Cell::findExact(const QString &s)
{
    return text.t == s ? this : (grid ? grid->findExact(s) : nullptr);
}

int Cell::getX(Document *doc) const
{
    return ox + (p ? p->getX(doc) : doc->hierarchysize);
}

int Cell::getY(Document *doc) const
{
    return oy + (p ? p->getY(doc) : doc->hierarchysize);
}
