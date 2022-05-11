#include "document.h"
#include "selection.h"
#include "grid.h"

Document::Document(Widget *sw)
    : sw(sw)
{
    undolistsizeatfullsave = 0;
    modified = false;
}

Document::~Document()
{
    // TODO
}

void Document::initWith(Cell *r, const QString &filename)
{
    // TODO
//    rootgrid = r;
//    selected = Selection(r->grid, 0, 0, 1, 1);
    //    ChangeFileName(filename, false);
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

void Document::drawSelectMove(QPainter &dc, Selection &s, bool refreshalways, bool refreshinstead) {
    if (scrollIfSelectionOutOfView(dc, s)) return;
    if (refreshalways) refreshReset();
    else drawSelect(dc, s, refreshinstead);
}

void Document::refresh()
{
    //    hover.g = nullptr;
    //    RefreshHover();
}

void Document::resetFont()
{
    // TODO
    //    lasttextsize = INT_MAX;
    //    laststylebits = -1;
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

