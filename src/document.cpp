#include "document.h"
#include "grid.h"
#include "tools.h"
#include "myapp.h"
#include "mainwindow.h"
#include "cell.h"
#include "widget.h"
#include "config.h"

#include <QFileInfo>
#include <QPainter>
#include <QDebug>
#include <QScrollArea>

Document::Document(Widget *sw)
{
    this->sw = sw;
    rootgrid = nullptr;
    lastmodsinceautosave = 0;
    undolistsizeatfullsave = 0;
    pathscalebias = 0;
    modified = false;
    redrawpending = false;
    while_printing = false;
    scaledviewingmode = false;
}

Document::~Document()
{
    // TODO
    //itercells.setsize_nd(0);
    DELPTR(rootgrid);
}

void Document::initWith(Cell *r, const QString &filename)
{
    rootgrid = r;
    selected = Selection(r->grid, 0, 0, 1, 1);
    changeFileName(filename, false);
}

void Document::clearSelectionRefresh()
{
    // TODO
}

bool Document::scrollIfSelectionOutOfView(QPainter &dc, Selection &s, bool refreshalways)
{
    // TODO
    return true;
//    if (!scaledviewingmode) {
//        // required, since sizes of things may have been reset by the last editing operation
//        Layout(dc);
//        int canvasw, canvash;
//        sw->GetClientSize(&canvasw, &canvash);
//        if ((layoutys > canvash || layoutxs > canvasw) && s.g) {
//            wxRect r = s.g->GetRect(this, s, true);
//            if (r.y < originy || r.y + r.height > maxy || r.x < originx ||
//                    r.x + r.width > maxx) {
//                int curx, cury;
//                sw->GetViewStart(&curx, &cury);
//                sw->SetScrollbars(1, 1, layoutxs, layoutys,
//                                  r.width > canvasw || r.x < originx
//                                  ? r.x
//                                  : r.x + r.width > maxx ? r.x + r.width - canvasw : curx,
//                                  r.height > canvash || r.y < originy
//                                  ? r.y
//                                  : r.y + r.height > maxy ? r.y + r.height - canvash : cury,
//                                  true);
//                RefreshReset();
//                return true;
//            }
//        }
//    }
//    if (refreshalways) Refresh();
//    return refreshalways;
}

void Document::drawSelect(QPainter &dc, Selection &s, bool refreshinstead, bool cursoronly)
{
#ifdef SIMPLERENDER
    if (refreshinstead) {
        refresh();
        return;
    }
#endif
    if (!s.g) return;
    resetFont();
    s.g->drawSelect(this, dc, s, cursoronly);
}

void Document::drawSelectMove(QPainter &dc, Selection &s, bool refreshalways, bool refreshinstead)
{
    if (scrollIfSelectionOutOfView(dc, s)) return;
    if (refreshalways) refreshReset();
    else drawSelect(dc, s, refreshinstead);
}

void Document::draw(QPainter &dc)
{
    redrawpending = false;
    //dc.setBackground();
    //dc.setBrush(QBrush(QColor(background())));

    if (!rootgrid) return;
    maxx = sw->scrollwin->viewport()->width();
    maxy = sw->scrollwin->viewport()->height();

    layout(dc);
    double xscale = maxx / (double)layoutxs;
    double yscale = maxy / (double)layoutys;
    currentviewscale = qMin(xscale, yscale);
    if (currentviewscale > 5)
        currentviewscale = 5;
    else if (currentviewscale < 1)
        currentviewscale = 1;
    if (scaledviewingmode && currentviewscale > 1)
    {
        sw->resize(maxx, maxy);
        dc.scale(currentviewscale, currentviewscale);
        maxx /= currentviewscale;
        maxy /= currentviewscale;
        originx = originy = 0;
    }
    else
    {
        currentviewscale = 1;
        dc.scale(1, 1);
        int drx = qMax(layoutxs, maxx);
        int dry = qMax(layoutys, maxy);
        sw->resize(drx, dry);
        const auto &p = sw->scrollwin->viewport()->rect().topLeft();
        originx = p.x();
        originy = p.y();
        maxx += originx;
        maxy += originy;
    }
    centerx = myApp.cfg->centered && !originx && maxx > layoutxs
                  ? (maxx - layoutxs) / 2 * currentviewscale
                  : 0;
    centery = myApp.cfg->centered && !originy && maxy > layoutys
                  ? (maxy - layoutys) / 2 * currentviewscale
                  : 0;
    // sw->DoPrepareDC(dc);
    shiftToCenter(dc);
    render(dc);
    drawSelect(dc, selected);
    if (hover.g) hover.g->drawHover(this, dc, hover);
    if (scaledviewingmode) { dc.scale(1, 1); }
}

void Document::layout(QPainter &dc)
{
    resetFont();
    curdrawroot = walkPath(drawpath);
    int psb = curdrawroot == rootgrid ? 0 : curdrawroot->minRelsize();
    if (psb < 0 || psb == INT_MAX) psb = 0;
    if (psb != pathscalebias) curdrawroot->resetChildren();
    pathscalebias = psb;
    curdrawroot->lazyLayout(this, dc, 0, curdrawroot->colWidth(), false);
    resetFont();
    pickFont(dc, 0, 0, 0);
    hierarchysize = 0;
    for (Cell *p = curdrawroot->p; p; p = p->p)
    {
        // TODO xHeight or height
        if (p->text.t.length()) hierarchysize += dc.fontMetrics().xHeight();
    }
    hierarchysize += fgutter;
    layoutxs = curdrawroot->sx + hierarchysize + fgutter;
    layoutys = curdrawroot->sy + hierarchysize + fgutter;
}

void Document::shiftToCenter(QPainter &dc)
{
    // ?? ==
    dc.translate(centerx, centery);
    dc.scale(currentviewscale, currentviewscale);
    dc.translate(-centerx, centery);

    // TODO
//    int dlx = dc.DeviceToLogicalX(0);
//    int dly = dc.DeviceToLogicalY(0);
//    dc.SetDeviceOrigin(dlx > 0 ? -dlx : centerx, dly > 0 ? -dly : centery);
//    dc.scale(currentviewscale, currentviewscale);
}

void Document::render(QPainter &dc)
{
    resetFont();
    pickFont(dc, 0, 0, 0);
    QPen pen(dc.pen());
    pen.setColor(Qt::lightGray);
    dc.setPen(pen);
    int i = 0;
    for (Cell *p = curdrawroot->p; p; p = p->p)
    {
        if (p->text.t.length())
        {
            // TODO
            int off = hierarchysize - dc.fontMetrics().xHeight() * ++i;
            QString s = p->text.t;
            if (s.length() > myApp.cfg->defaultmaxcolwidth)
            {
                // should take the width of these into account for layoutys, but really, the
                // worst that can happen on a thin window is that its rendering gets cut off
                s = s.left(myApp.cfg->defaultmaxcolwidth) + QStringLiteral("...");
            }
            dc.drawText(off, off, s);
        }
    }
    pen.setColor(Qt::black);
    dc.setPen(pen);
    curdrawroot->render(this, hierarchysize, hierarchysize, dc, 0, 0, 0, 0, 0,
                        curdrawroot->colWidth(), 0);
}

void Document::refresh()
{
    //    hover.g = nullptr;
    //    RefreshHover();
}

void Document::resetFont()
{
    lasttextsize = INT_MAX;
    laststylebits = -1;
}

void Document::addUndo(Cell *c)
{
    //    redolist.setsize(0);
    //    lastmodsinceautosave = wxGetLocalTime();
    //    if (!modified) {
    //        modified = true;
    //        UpdateFileName();
    //    }
    //    if (LastUndoSameCell(c)) return;
    //    UndoItem *ui = new UndoItem();
    //    undolist.push() = ui;
    //    ui->clone = c->Clone(nullptr);
    //    ui->estimated_size = c->EstimatedMemoryUse();
    //    ui->sel = selected;
    //    CreatePath(c, ui->path);
    //    if (selected.g) CreatePath(selected.g->cell, ui->selpath);
    //    size_t total_usage = 0;
    //    size_t old_list_size = undolist.size();
    //    // Cull undolist. Always at least keeps last item.
    //    for (int i = (int)undolist.size() - 1; i >= 0; i--) {
    //        // Cull old items if using more than 100MB or 1000 items, whichever comes first.
//        // TODO: make configurable?
//        if (total_usage < 100 * 1024 * 1024 && undolist.size() - i < 1000) {
//            total_usage += undolist[i]->estimated_size;
//        } else {
//            undolist.remove(0, i + 1);
//            break;
//        }
//    }
//    size_t items_culled = old_list_size - undolist.size();
//    undolistsizeatfullsave -= items_culled;  // Allowed to go < 0
}

void Document::changeFileName(const QString &fn, bool checkext)
{
    filename = fn;
    if (checkext && QFileInfo(filename).suffix().isEmpty())
    {
        filename.append(QStringLiteral(".cts"));
    }
    updateFileName();
}

void Document::updateFileName(int page)
{
    QString mode;
    if (modified)
    {
        if (lastmodsinceautosave)
        {
            mode = QStringLiteral("*");
        }
        else
        {
            mode = QStringLiteral("+");
        }
    }
    myApp.frame->setPageTitle(filename, mode, page);
}

uint Document::background() const
{
    return rootgrid ? rootgrid->cellcolor : 0xFFFFFF;
}

Cell *Document::walkPath(const QVector<Selection> &path)
{
    Cell *c = rootgrid;
    for (int i = path.size() - 1; i >= 0; i--)
    {
        const Selection &s = path[i];
        Grid *g = c->grid;
        Q_ASSERT(g && s.x < g->xs && s.y < g->ys);
        c = g->C(s.x, s.y);
    }
    return c;
}

bool Document::pickFont(QPainter &dc, int depth, int relsize, int stylebits)
{
    int textsize = textSize(depth, relsize);
    if (textsize != lasttextsize || stylebits != laststylebits)
    {
        QFont font(dc.font());
        font.setPointSize(textsize - (while_printing || scaledviewingmode));
        font.setStyleHint(stylebits & STYLE_FIXED? QFont::TypeWriter: QFont::System);
        font.setItalic(stylebits & STYLE_ITALIC);
        font.setBold(stylebits & STYLE_BOLD);
        font.setUnderline(stylebits & STYLE_UNDERLINE);
        font.setStrikeOut(stylebits & STYLE_STRIKETHRU);
        if (!(stylebits & STYLE_FIXED)) font.setFamily(myApp.cfg->defaultfont);
        dc.setFont(font);
        lasttextsize = textsize;
        laststylebits = stylebits;
    }
    return fontIsMini(textsize);
}

int Document::textSize(int depth, int relsize)
{
    return qMax(_g::mintextsize(), _g::deftextsize - depth - relsize + pathscalebias);
}

bool Document::fontIsMini(int textsize) const
{
    return textsize == _g::mintextsize();
}

const QString Document::wheel(QPainter &dc, int dir, bool alt, bool ctrl, bool shift, bool hierarchical)
{
//    if (!dir) return QString();
//    shiftToCenter(dc);
//    if (alt)
//    {
//        if (!selected.g) return NoSel();
//        if (selected.xs > 0) {
//            // FIXME: should do undo, but this is a lot of undos that need to coalesced, same
//            // for relsize
//            selected.g->ResizeColWidths(dir, selected, hierarchical);
//            selected.g->cell->ResetLayout();
//            selected.g->cell->ResetChildren();
//            sys->UpdateStatus(selected);
//            Refresh();
//            return dir > 0 ? _(L"Column width increased.") : _(L"Column width decreased.");
//        }
//        return L"nothing to resize";
//    } else if (shift) {
//        if (!selected.g) return NoSel();
//        selected.g->cell->AddUndo(this);
//        selected.g->ResetChildren();
//        selected.g->RelSize(-dir, selected, pathscalebias);
//        sys->UpdateStatus(selected);
//        Refresh();
//        return dir > 0 ? _(L"Text size increased.") : _(L"Text size decreased.");
//    } else if (ctrl) {
//        int steps = abs(dir);
//        dir = sign(dir);
//        loop(i, steps) Zoom(dir, dc);
//        return dir > 0 ? _(L"Zoomed in.") : _(L"Zoomed out.");
//    } else {
//        ASSERT(0);
//        return nullptr;
//    }
    return QString();
}

void Document::Hover(int x, int y, QPainter &dc)
{
//    if (redrawpending) return;
//    shiftToCenter(dc);
//    resetFont();
//    Selection prev = hover;
//    hover = Selection();
//    auto drawroot = walkPath(drawpath);
//    if (drawroot->grid) drawroot->grid->findXY(this, x - centerx / currentviewscale - hierarchysize,
//                                               y - centery / currentviewscale - hierarchysize, dc);
//    if (!(prev == hover)) {
//        if (prev.g) prev.g->DrawHover(this, dc, prev);
//        if (hover.g) hover.g->DrawHover(this, dc, hover);
//    }
//    sys->UpdateStatus(hover);
}
