#include "selection.h"
#include "document.h"
#include "widget.h"
#include "cell.h"
#include "grid.h"
#include "text.h"
#include "myapp.h"
#include "config.h"

Selection::Selection()
{
    memset(this, 0, sizeof(Selection));
}

Selection::Selection(Grid *_g, int _x, int _y, int _xs, int _ys)
{
    g = _g;
    x = _x;
    y = _y;
    xs = _xs;
    ys = _ys;
    cursor = 0;
    cursorend = 0;
    firstdx = 0;
    firstdy = 0;
    textedit = false;
}

void Selection::setCursorEdit(Document *doc, bool edit)
{
    auto c = edit? Qt::IBeamCursor: Qt::ArrowCursor;
    // TODO
//#ifdef  WIN32
//    // this changes the cursor instantly, but gets overridden by the local window cursor
//    ::SetCursor((HCURSOR)c.GetHCURSOR());
//#endif
//    // this doesn't change the cursor immediately, only on mousemove:
    doc->sw->setCursor(c);
    firstdx = firstdy = 0;
}

void Selection::enterEditOnly(Document *doc)
{
    textedit = true;
    setCursorEdit(doc, true);
}

void Selection::enterEdit(Document *doc, int c, int ce)
{
    enterEditOnly(doc);
    cursor = c;
    cursorend = ce;
}

void Selection::exitEdit(Document *doc)
{
    textedit = false;
    cursor = cursorend = 0;
    setCursorEdit(doc, false);
}

bool Selection::isInside(const Selection &o) const
{
    if (!o.g || !g) return false;
    if (g != o.g)
    {
        return g->cell->p && g->cell->p->grid->findCell(g->cell).isInside(o);
    }
    return x >= o.x && y >= o.y && x + xs <= o.x + o.xs && y + ys <= o.y + o.ys;
}

void Selection::merge(const Selection &a, const Selection &b)
{
    textedit = false;
    if (a.g == b.g)
    {
        if (a.getCell() == b.getCell() && a.getCell() && (a.textedit || b.textedit))
        {
            if (a.cursor != a.cursorend)
            {
                Selection c = b;
                a.getCell()->text.selectWord(c);
                cursor = qMin(a.cursor, c.cursor);
                cursorend = qMax(a.cursorend, c.cursorend);
            }
            else
            {
                cursor = qMin(a.cursor, b.cursor);
                cursorend = qMax(a.cursor, b.cursor);
            }
            textedit = true;
        }
        else
        {
            cursor = cursorend = 0;
        }
        g = a.g;
        x = qMin(a.x, b.x);
        y = qMin(a.y, b.y);
        xs = abs(a.x - b.x) + 1;
        ys = abs(a.y - b.y) + 1;
    }
    else
    {
        Cell *at = a.getCell();
        Cell *bt = b.getCell();
        int ad = at->depth();
        int bd = bt->depth();
        int i = 0;
        while (i < ad && i < bd && at->parent(ad - i) == bt->parent(bd - i))
        {
            i++;
        }
        Grid *g = at->parent(ad - i + 1)->grid;
        merge(g->findCell(at->parent(ad - i)), g->findCell(bt->parent(bd - i)));
    }
}

int Selection::maxCursor() const
{
    return getCell()->text.t.length();
}

void Selection::dir(Document *doc, bool ctrl, bool shift, QPainter &dc, int dx, int dy, int &v, int &vs,
                    int &ovs, bool notboundaryperp, bool notboundarypar, bool exitedit)
{
    if (ctrl && !textedit)
    {
        g->cell->addUndo(doc);

        g->move(dx, dy, *this);
        x = (x + dx + g->xs) % g->xs;
        y = (y + dy + g->ys) % g->ys;
        if (x + xs > g->xs || y + ys > g->ys) g = nullptr;

        doc->scrollIfSelectionOutOfView(dc, *this, true);
        return;
    }

    doc->drawSelect(dc, *this);
    if (ctrl && dx)  // implies textedit
    {
        if (cursor == cursorend) firstdx = dx;
        int &curs = firstdx < 0 ? cursor : cursorend;
        for (int c = curs, start = curs;;)
        {
            c += dx;
            if (c < 0 || c > maxCursor()) break;
            QChar ch = getCell()->text.t.at((c + curs) / 2);
            if (!ch.isLetterOrNumber() && curs != start) break;
            curs = c;
            if (!ch.isLetterOrNumber() && !ch.isSpace()) break;
        }
        if (shift)
        {
            if (cursorend < cursor) qSwap(cursorend, cursor);
        }
        else cursorend = cursor = curs;
    }
    else if (shift)
    {
        if (textedit)
        {
            if (cursor == cursorend) firstdx = dx;
            (firstdx < 0 ? cursor : cursorend) += dx;
            if (cursor < 0) cursor = 0;
            if (cursorend > maxCursor()) cursorend = maxCursor();
        }
        else
        {
            if (!xs) firstdx = 0;  // redundant: just in case someone else changed it
            if (!ys) firstdy = 0;
            if (!firstdx) firstdx = dx;
            if (!firstdy) firstdy = dy;
            if (firstdx < 0)
            {
                x += dx;
                xs += -dx;
            }
            else xs += dx;
            if (firstdy < 0)
            {
                y += dy;
                ys += -dy;
            }
            else ys += dy;
            if (x < 0)
            {
                x = 0;
                xs--;
            }
            if (y < 0)
            {
                y = 0;
                ys--;
            }
            if (x + xs > g->xs) xs--;
            if (y + ys > g->ys) ys--;
            if (!xs) firstdx = 0;
            if (!ys) firstdy = 0;
            if (!xs && !ys) g = nullptr;
        }
    }
    else
    {
        if (vs)
        {
            if (ovs)  // (multi) cell selection
            {
                bool intracell = true;
                if (textedit && !exitedit && getCell())
                {
                    if (dy) {
                        cursorend = cursor;
                        Text &text = getCell()->text;
                        int maxcolwidth = getCell()->p->grid->colwidths[x];

                        int i = 0;
                        int laststart, lastlen;
                        int nextoffset = -1;
                        for (int l = 0;; l++)
                        {
                            int start = i;
                            QString ls = text.getLine(i, maxcolwidth);
                            int len = ls.length();
                            int end = start + len;

                            if (len && nextoffset >= 0)
                            {
                                cursor = cursorend =
                                        start + (nextoffset > len ? len : nextoffset);
                                intracell = false;
                                break;
                            }

                            if (cursor >= start && cursor <= end)
                            {
                                if (dy < 0)
                                {
                                    if (l != 0)
                                    {
                                        cursor = cursorend =
                                                laststart + (cursor - start > lastlen
                                                             ? lastlen
                                                             : cursor - start);
                                        intracell = false;
                                    }
                                    break;
                                }
                                else
                                {
                                    nextoffset = cursor - start;
                                }
                            }

                            laststart = start;
                            lastlen = len;

                            if (!len) break;
                        }
                    }
                    else
                    {
                        intracell = false;
                        if (cursor != cursorend)
                        {
                            if (dx < 0) cursorend = cursor;
                            else cursor = cursorend;
                        }
                        else if ((dx < 0 && cursor) || (dx > 0 && maxCursor() > cursor))
                        {
                            cursorend = cursor += dx;
                        }

                    }
                }

                if (intracell)
                {
                    if (myApp.cfg->thinselc)
                    {
                        if (dx + dy > 0) v += vs;
                        vs = 0;  // make it a thin selection, in direction
                        ovs = 1;
                    }
                    else
                    {
                        if (x + dx >= 0 && x + dx + xs <= g->xs && y + dy >= 0 &&
                                y + dy + ys <= g->ys)
                        {
                            x += dx;
                            y += dy;
                        }
                    }
                    exitEdit(doc);
                }
            }
            else if (notboundarypar)  // thin selection, moving in parallel direction
            {
                v += dx + dy;
            }
        }
        else if (notboundaryperp)  // thin selection, moving in perpendicular direction
        {
            if (dx + dy < 0) v--;
            vs = 1;  // make it a cell selection
        }
    }
    doc->drawSelectMove(dc, *this);
}

void Selection::Cursor(Document *doc, int k, bool ctrl, bool shift, QPainter &dc, bool exitedit)
{
    switch (k) {
    case A_UP:
        dir(doc, ctrl, shift, dc, 0, -1, y, ys, xs, y != 0, y != 0, exitedit);
        break;
    case A_DOWN:
        dir(doc, ctrl, shift, dc, 0, 1, y, ys, xs, y < g->ys, y < g->ys - 1, exitedit);
        break;
    case A_LEFT:
        dir(doc, ctrl, shift, dc, -1, 0, x, xs, ys, x != 0, x != 0, exitedit);
        break;
    case A_RIGHT:
        dir(doc, ctrl, shift, dc, 1, 0, x, xs, ys, x < g->xs, x < g->xs - 1, exitedit);
        break;
    }
}

void Selection::next(Document *doc, QPainter &dc, bool backwards)
{
    doc->drawSelect(dc, *this);
    exitEdit(doc);
    if (backwards)
    {
        if (x > 0) x--;
        else if (y > 0)
        {
            y--;
            x = g->xs - 1;
        }
        else
        {
            x = g->xs - 1;
            y = g->ys - 1;
        }
    }
    else
    {
        if (x < g->xs - 1) x++;
        else if (y < g->ys - 1)
        {
            y++;
            x = 0;
        }
        else x = y = 0;
    }
    enterEdit(doc, 0, maxCursor());
    doc->drawSelectMove(dc, *this);
}

const QChar *Selection::wrap(Document *doc)
{
    // TODO
    return nullptr;
//    if (thin()) return doc->NoThin();
//    g->cell->AddUndo(doc);
//    auto np = g->CloneSel(*this).release();
//    g->C(x, y)->text.t = ".";  // avoid this cell getting deleted
//    if (xs > 1) {
//        Selection s(g, x + 1, y, xs - 1, ys);
//        g->MultiCellDeleteSub(doc, s);
//    }
//    if (ys > 1) {
//        Selection s(g, x, y + 1, 1, ys - 1);
//        g->MultiCellDeleteSub(doc, s);
//    }
//    Cell *old = g->C(x, y);
//    np->text.relsize = old->text.relsize;
//    np->CloneStyleFrom(old);
//    g->ReplaceCell(old, np);
//    np->parent = g->cell;
//    delete old;
//    xs = ys = 1;
//    EnterEdit(doc, maxCursor(), maxCursor());
//    doc->refresh();
//    return nullptr;
}

Cell *Selection::thinExpand(Document *doc)
{
    if (thin())
    {
        if (xs) {
            g->cell->addUndo(doc);
            g->insertCells(-1, y, 0, 1);
            ys = 1;
        } else {
            g->cell->addUndo(doc);
            g->insertCells(x, -1, 1, 0);
            xs = 1;
        }
    }
    return getCell();
}

void Selection::homeEnd(Document *doc, QPainter &dc, bool ishome)
{
    doc->drawSelect(dc, *this);
    xs = ys = 1;
    if (ishome) x = y = 0;
    else
    {
        x = g->xs - 1;
        y = g->ys - 1;
    }
    doc->drawSelectMove(dc, *this);
}

void Selection::selAll()
{
    x = y = 0;
    xs = g->xs;
    ys = g->ys;
}

Cell *Selection::getCell() const
{
    return g && xs == 1 && ys == 1 ? g->C(x, y) : nullptr;
}

Cell *Selection::getFirst() const
{
    return g && xs >= 1 && ys >= 1 ? g->C(x, y) : nullptr;
}

bool Selection::isAll() const
{
    return xs == g->xs && ys == g->ys;
}

bool Selection::eqLoc(const Selection &s) const
{
    return g == s.g && x == s.x && y == s.y && xs == s.xs && ys == s.ys;
}

bool Selection::operator==(const Selection &s) const
{
    return eqLoc(s) && cursor == s.cursor && cursorend == s.cursorend;
}

bool Selection::textEdit() const
{
    return textedit;
}

bool Selection::thin() const
{
    return !(xs && ys);
}
