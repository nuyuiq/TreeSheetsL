#include "document.h"
#include "grid.h"
#include "tools.h"
#include "myapp.h"
#include "mainwindow.h"
#include "cell.h"
#include "widget.h"
#include "config.h"
#include "widget.h"
#include "history.h"
#include "dropdown.h"

#include <QFileInfo>
#include <QPainter>
#include <QDebug>
#include <QScrollBar>
#include <QScrollArea>
#include <QMessageBox>
#include <QBuffer>
#include <QFileDialog>
#include <QInputDialog>
#include <QDesktopServices>
#include <QFontDialog>
#include <QColorDialog>
#include <QGuiApplication>
#include <QClipboard>


#define loopcellsin(par, c) \
    collectCells(par);      \
    for (int _i = 0; _i < itercells.size(); _i++) \
        for (Cell *c = itercells[_i]; c; c = nullptr)
#define loopallcells(c)     \
    collectCells(rootgrid); \
    for (int _i = 0; _i < itercells.size(); _i++) \
        for (Cell *c = itercells[_i]; c; c = nullptr)
#define loopallcellssel(c, rec) \
    collectCellsSel(rec);     \
    for (int _i = 0; _i < itercells.size(); _i++) \
        for (Cell *c = itercells[_i]; c; c = nullptr)


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
    fgutter = 6;
    isctrlshiftdrag = 0;
    blink = true;
    tmpsavesuccess = true;
    printscale = 0;
    editfilter = 0;
    searchfilter = false;
    lastsave = QDateTime::currentMSecsSinceEpoch() / 1000;
}

Document::~Document()
{
    qDeleteAll(undolist);
    qDeleteAll(redolist);
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
    selected.g = nullptr;
    refresh();
}

bool Document::scrollIfSelectionOutOfView(QPainter &dc, Selection &s, bool refreshalways)
{
    // TODO
    if (!scaledviewingmode)
    {
        // required, since sizes of things may have been reset by the last editing operation
        layout(dc);
        const auto &ps = sw->scrollwin->viewport()->size();
        int canvasw = ps.width(), canvash = ps.height();
        if ((layoutys > canvash || layoutxs > canvasw) && s.g)
        {
            QRect r = s.g->getRect(this, s, true);
            if (r.y() < originy || r.y() + r.height() > maxy || r.x() < originx ||
                    r.x() + r.width() > maxx)
            {
                const auto &pp = sw->pos();
                int curx = -pp.x(), cury = -pp.y();
                sw->resize(layoutxs, layoutys);
                auto dx = r.width() > canvasw || r.x() < originx?
                            r.x():
                            r.x() + r.width() > maxx ?
                                r.x() + r.width() - canvasw:
                                curx;
                auto dy = r.height() > canvash || r.y() < originy?
                            r.y():
                            r.y() + r.height() > maxy?
                                r.y() + r.height() - canvash:
                                cury;
                auto bar = sw->scrollwin->verticalScrollBar();
                bar->setValue(dy);
                bar = sw->scrollwin->horizontalScrollBar();
                bar->setValue(dx);
                refreshReset();
                return true;
            }
        }
    }
    if (refreshalways) refresh();
    return refreshalways;
}

void Document::drawSelect(QPainter &dc, Selection &s, bool refreshinstead, bool cursoronly)
{
#ifdef SIMPLERENDER
    if (refreshinstead)
    {
        refresh();
        return;
    }
#else
    Q_UNUSED(refreshinstead)
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
    dc.fillRect(dc.clipBoundingRect(), Color(background()));

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
        const auto &p = sw->pos();
        originx = -p.x();
        originy = -p.y();
        maxx += originx;
        maxy += originy;
    }

    centerx = myApp.cfg->centered && !originx && maxx > layoutxs
                  ? (maxx - layoutxs) / 2 * currentviewscale
                  : 0;
    centery = myApp.cfg->centered && !originy && maxy > layoutys
                  ? (maxy - layoutys) / 2 * currentviewscale
                  : 0;

    // 校准绘制区
    auto roi = dc.clipBoundingRect().toRect();
    if (roi.width() == 0 || roi.height() == 0) return;
    else
    {
        const auto &o = QRect(originx, originy, maxx - originx, maxy - originy);
        const auto &tf = QTransform()  // TODO 顺序
                .scale(1/currentviewscale, 1/currentviewscale)
                .translate(-centerx, -centery)
                ;
        const auto &n = o & tf.mapRect(roi);
        n.getCoords(&originx, &originy, &maxx, &maxy);
    }

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
        if (p->text.t.length()) hierarchysize += dc.fontMetrics().height();
    }
    hierarchysize += fgutter;
    layoutxs = curdrawroot->sx + hierarchysize + fgutter;
    layoutys = curdrawroot->sy + hierarchysize + fgutter;
}

void Document::shiftToCenter(QPainter &dc)
{
    // 已经进行过变换了
    if (dc.transform().map(QPoint(1, 1)) != QPoint(1, 1)) return;
    const auto &ps = dc.viewport().topLeft();
    dc.translate(ps.x() > 0? -ps.x(): centerx, ps.y() > 0? ps.y(): centery);
    dc.scale(currentviewscale, currentviewscale);
}

void Document::render(QPainter &dc)
{
    resetFont();
    pickFont(dc, 0, 0, 0);
    QPen pen(dc.pen());
    pen.setColor(Qt::lightGray);
    dc.setPen(pen);
    int i = 0;
    const int cHeight = dc.fontMetrics().height();
    const int cAscent = dc.fontMetrics().ascent();
    for (Cell *p = curdrawroot->p; p; p = p->p)
    {
        if (p->text.t.length())
        {
            int off = hierarchysize - cHeight * ++i;
            QString s = p->text.t;
            if (s.length() > myApp.cfg->defaultmaxcolwidth)
            {
                // should take the width of these into account for layoutys, but really, the
                // worst that can happen on a thin window is that its rendering gets cut off
                s = s.left(myApp.cfg->defaultmaxcolwidth) + QStringLiteral("...");
            }
            dc.drawText(off, off + cAscent, s);
        }
    }
    pen.setColor(Qt::black);
    dc.setPen(pen);
    curdrawroot->render(this, hierarchysize, hierarchysize, dc, 0, 0, 0, 0, 0,
                        curdrawroot->colWidth(), 0);
}

void Document::refresh()
{
    hover.g = nullptr;
    refreshHover();
}

void Document::resetFont()
{
    lasttextsize = INT_MAX;
    laststylebits = -1;
}

void Document::addUndo(Cell *c)
{
    if (redolist.size())
    {
        qDeleteAll(redolist);
        redolist.clear();
    }

    lastmodsinceautosave = QDateTime::currentMSecsSinceEpoch() / 1000;
    if (!modified)
    {
        modified = true;
        updateFileName();
    }
    if (lastUndoSameCell(c)) return;
    UndoItem *ui = new UndoItem();
    undolist.append(ui);
    ui->clone = c->clone(nullptr);
    ui->sel = selected;
    createPath(c, ui->path);
    if (selected.g) createPath(selected.g->cell, ui->selpath);
    size_t old_list_size = undolist.size();
    while (undolist.size() > 1000)
    {
        delete undolist.takeFirst();
    }
    size_t items_culled = old_list_size - undolist.size();
    undolistsizeatfullsave -= items_culled;  // Allowed to go < 0
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
    if (!dir) return QString();
    shiftToCenter(dc);
    if (alt)
    {
        if (!selected.g) return noSel();
        if (selected.xs > 0)
        {
            // FIXME: should do undo, but this is a lot of undos that need to coalesced, same
            // for relsize
            selected.g->resizeColWidths(dir, selected, hierarchical);
            selected.g->cell->resetLayout();
            selected.g->cell->resetChildren();
            myApp.frame->updateStatus(selected);
            refresh();
            return dir > 0 ? tr("Column width increased.") : tr("Column width decreased.");
        }
        return tr("nothing to resize");
    }
    else if (shift)
    {
        if (!selected.g) return noSel();
        selected.g->cell->addUndo(this);
        selected.g->resetChildren();
        selected.g->relSize(-dir, selected, pathscalebias);
        myApp.frame->updateStatus(selected);
        refresh();
        return dir > 0 ? tr("Text size increased.") : tr("Text size decreased.");
    }
    else if (ctrl)
    {
        int steps = qAbs(dir);
        dir = dir > 0? 1: -1; // TODO
        for (int i = 0; i < steps; i++) zoom(dir, dc);
        return dir > 0 ? tr("Zoomed in.") : tr("Zoomed out.");
    }
    Q_ASSERT(false);
    return QString();
}

void Document::Hover(int x, int y, QPainter &dc)
{
    if (redrawpending) return;
    shiftToCenter(dc);
    resetFont();
    Selection prev = hover;
    hover = Selection();
    auto drawroot = walkPath(drawpath);
    if (drawroot->grid) drawroot->grid->findXY(this, x - centerx / currentviewscale - hierarchysize,
                                               y - centery / currentviewscale - hierarchysize, dc);
    if (!(prev == hover))
    {
        if (prev.g) prev.g->drawHover(this, dc, prev);
        if (hover.g) hover.g->drawHover(this, dc, hover);
    }
    myApp.frame->updateStatus(hover);
}

void Document::select(QPainter &dc, bool right, int isctrlshift)
{
    begindrag = Selection();
    if (right && hover.isInside(selected)) return;
    shiftToCenter(dc);
    drawSelect(dc, selected);
    if (selected.getCell() == hover.getCell() && hover.getCell())
    {
        hover.enterEditOnly(this);
    }
    selected = hover;
    begindrag = hover;
    isctrlshiftdrag = isctrlshift;
    drawSelectMove(dc, selected);
    resetCursor();
    return;
}

void Document::selectUp()
{
    if (!isctrlshiftdrag || isctrlshiftdrag == 3 || begindrag.eqLoc(selected)) return;
    Cell *c = selected.getCell();
    if (!c) return;
    Cell *tc = begindrag.thinExpand(this);
    selected = begindrag;
    if (tc)
    {
        auto is_parent = tc->isParentOf(c);
        auto tc_parent = tc->p;  // tc may be deleted.
        tc->paste(this, c, begindrag);
        // If is_parent, c has been deleted already.
        if (isctrlshiftdrag == 1 && !is_parent)
        {
            c->p->addUndo(this);
            Selection cs = c->p->grid->findCell(c);
            c->p->grid->multiCellDeleteSub(this, cs);
        }
        hover = selected = tc_parent ? tc_parent->grid->findCell(tc) : Selection();
    }
    refresh();
}

void Document::doubleClick(QPainter &dc)
{
    if (!selected.g) return;
    shiftToCenter(dc);
    Cell *c = selected.getCell();
    if (selected.thin())
    {
        selected.selAll();
        refresh();
    }
    else if (c)
    {
        drawSelect(dc, selected);
        if (selected.textEdit()) {
            c->text.selectWord(selected);
            begindrag = selected;
        }
        else
        {
            selected.enterEditOnly(this);
        }
        drawSelect(dc, selected, true);
    }
}

void Document::refreshHover()
{
    redrawpending = true;
    myApp.frame->updateStatus(selected);
    myApp.frame->nb->update();
}

void Document::zoom(int dir, QPainter &dc, bool fromroot, bool selectionmaybedrawroot)
{
    int len = qMax(0, (fromroot ? 0 : drawpath.size()) + dir);
    if (!len && !drawpath.size()) return;
    if (dir > 0)
    {
        if (!selected.g) return;
        Cell *c = selected.getCell();
        createPath(c && c->grid ? c : selected.g->cell, drawpath);
    }
    else if (dir < 0)
    {
        Cell *drawroot = walkPath(drawpath);
        if (drawroot->grid && drawroot->grid->folded && selectionmaybedrawroot)
        {
            selected = drawroot->p->grid->findCell(drawroot);
        }
    }
    while (len < drawpath.size()) drawpath.remove(0);
    Cell *drawroot = walkPath(drawpath);
    if (selected.getCell() == drawroot && drawroot->grid)
    {
        selected = Selection(drawroot->grid, 0, 0, drawroot->grid->xs, drawroot->grid->ys);
    }
    drawroot->resetLayout();
    drawroot->resetChildren();
    layout(dc);
    drawSelectMove(dc, selected, true, false);
}

void Document::createPath(Cell *c, QVector<Selection> &path)
{
    path.clear();
    while (c->p)
    {
        const Selection &s = c->p->grid->findCell(c);
        Q_ASSERT(s.g);
        path.push_back(s);
        c = c->p;
    }
}

void Document::Blink()
{
    if (redrawpending) return;
#ifndef SIMPLERENDER
    Tools::Painter dc(sw);
    shiftToCenter(dc);
    drawSelect(dc, selected, false, true);
    blink = !blink;
    drawSelect(dc, selected, true, true);
#endif
}

void Document::drag(QPainter &dc)
{
    if (!selected.g || !hover.g || !begindrag.g) return;
    if (isctrlshiftdrag)
    {
        begindrag = hover;
        return;
    }
    if (hover.thin()) return;
    shiftToCenter(dc);
    if (begindrag.thin() || selected.thin())
    {
        drawSelect(dc, selected);
        begindrag = selected = hover;
        drawSelect(dc, selected, true);
    }
    else
    {
        Selection old = selected;
        selected.merge(begindrag, hover);
        if (!(old == selected))
        {
            drawSelect(dc, old);
            drawSelect(dc, selected, true);
        }
    }
    resetCursor();
    return;
}

bool Document::lastUndoSameCell(Cell *c) const
{
    // hacky way to detect word boundaries to stop coalescing, but works, and
    // not a big deal if selected is not actually related to this cell
    return undolist.size() && !c->grid && undolist.size() != undolistsizeatfullsave &&
            undolist.last()->sel.eqLoc(c->p->grid->findCell(c)) &&
            (!c->text.t.endsWith(QChar::fromLatin1(' ')) || c->text.t.length() != selected.cursor);
}

QString Document::save(bool saveas, bool *success)
{
    if (!saveas && !filename.isEmpty())
    {
        return saveDB(success);
    }
    const QString fn = QFileDialog::getSaveFileName(
                myApp.frame,
                QStringLiteral("Choose TreeSheets file to save:"),
                QString(),
                QStringLiteral("TreeSheets Files (*.cts);;All Files (*.*)"));
    if (fn.isEmpty())
    {
        return tr("Save cancelled.");  // avoid name being set to ""
    }
    changeFileName(fn, true);
    return saveDB(success);
}

void Document::autoSave(bool minimized, int page)
{
    if (tmpsavesuccess && !filename.isEmpty() && lastmodsinceautosave)
    {
        auto lt = QDateTime::currentMSecsSinceEpoch() / 1000;
        if (lastmodsinceautosave + 60 < lt || lastsave + 300 < lt || minimized)
        {
            tmpsavesuccess = false;
            saveDB(&tmpsavesuccess, true, page);
        }
    }
}

QString Document::saveDB(bool *success, bool istempfile, int page)
{
    if (filename.isEmpty()) return tr("Save cancelled.");

    {
        if (!istempfile && myApp.cfg->makebaks && QFile::exists(filename))
        {
            QFile::rename(filename, Tools::bakName(filename));
        }
        QString sfn = istempfile ? Tools::tmpName(filename) : filename;
        QFile file(sfn);
        if (!file.open(QIODevice::WriteOnly))
        {
            if (!istempfile)
            {
                QMessageBox::information(myApp.frame, sfn, tr("Error writing TreeSheets file! (try saving under new filename)."));
            }
            return tr("Error writing to file.");
        }
        Tools::DataIO sos(&file);
        sos.write("TSFF", 4);
        char vers = TS_VERSION;
        sos.write(&vers, 1);
        QVector<ImagePtr> imagelist;
        rootgrid->imageRefCollect(imagelist);
        for (auto itr = imagelist.constBegin(); itr != imagelist.constEnd(); itr++)
        {
            Image *img = itr->data();
            sos.write("I", 1);
            sos.writeDouble(img->display_scale);
            img->bm_orig.save(sos.d(), "PNG");
        }
        sos.write("D", 1);
        QByteArray buff;
        QBuffer dbuff(&buff);
        if (!dbuff.open(QIODevice::WriteOnly)) return tr("Zlib error while writing file.");
        Tools::DataIO dos(&dbuff);
        rootgrid->save(dos, imagelist);
        for (auto tagit = tags.constBegin(); tagit != tags.constEnd(); tagit++)
        {
            dos.writeString(*tagit);
        }
        dos.writeString(QString());
        auto ds = qCompress(buff, 9);
        sos.write(ds.constData() + 4, ds.size() - 4);
    }
    lastmodsinceautosave = 0;
    lastsave = QDateTime::currentMSecsSinceEpoch() / 1000;
    if (!istempfile)
    {
        undolistsizeatfullsave = undolist.size();
        modified = false;
        tmpsavesuccess = true;
        myApp.fileUsed(filename, this);
        if (QFile::exists(Tools::tmpName(filename)))
        {
            QFile::remove(Tools::tmpName(filename));
        }
    }
    if (myApp.cfg->autohtmlexport)
    {
        exportFile(Tools::extName(filename, QStringLiteral(".html")), A_EXPHTMLT, false);
    }
    updateFileName(page);
    if (success) *success = true;
    return tr("File saved succesfully.");
}

QString Document::exportFile(const QString &fn, int k, bool currentview)
{
    auto root = currentview ? curdrawroot : rootgrid;
    if (k == A_EXPCSV)
    {
        int maxdepth = 0, leaves = 0;
        root->maxDepthLeaves(0, maxdepth, leaves);
        if (maxdepth > 1)
        {
            return tr("Cannot export grid that is not flat (zoom the view to the desired grid, and/or use Flatten).");
        }
    }
    if (k == A_EXPIMAGE)
    {
        maxx = layoutxs;
        maxy = layoutys;
        originx = originy = 0;
        QPixmap bm(maxx, maxy);
        QPainter mdc(&bm);
        mdc.setClipRect(QRect(QPoint(0, 0), bm.size()));
        Tools::drawRect(mdc, background(), 0, 0, maxx, maxy);
        render(mdc);
        refresh();
        if (!bm.save(fn, "PNG"))
        {
            return tr("Error writing PNG file!");
        }
    }
    else
    {
        QFile fos(fn);
        if (!fos.open(QIODevice::WriteOnly))
        {
            QMessageBox::information(myApp.frame, fn, tr("Error exporting file!"));
            return tr("Error writing to file!");
        }
        QTextStream dos(&fos);
        QString content = root->toText(0, Selection(), k, this);
        switch (k)
        {
        case A_EXPXML:
            dos << QStringLiteral(
                        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<!DOCTYPE cell [\n"
                        "<!ELEMENT cell (grid)>\n"
                        "<!ELEMENT grid (row*)>\n"
                        "<!ELEMENT row (cell*)>\n"
                        "]>\n");
            dos << content;
            break;
        case A_EXPHTMLT:
        case A_EXPHTMLB:
        case A_EXPHTMLO:
            dos << QStringLiteral(
                        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
                        "<html>\n<head>\n<style>\n"
                        "body { font-family: sans-serif; }\n"
                        "table, th, td { border: 1px solid grey; border-collapse: collapse;"
                        " padding: 3px; }\n"
                        "li { }\n</style>\n"
                        "<title>export of TreeSheets file ");
            dos << filename;
            dos << QStringLiteral(
                        "</title>\n<meta http-equiv=\"Content-Type\" content=\"text/html; "
                        "charset=UTF-8\" "
                        "/>\n</head>\n<body>\n");
            dos << content;
            dos << QStringLiteral("</body>\n</html>\n");
            break;
        case A_EXPCSV:
        case A_EXPTEXT: dos << content; break;
        }
    }
    return tr("File exported successfully.");
}

QString Document::Export(const QString &fmt, const QString &pat, const QString &msg, int k)
{
    Q_UNUSED(fmt) // TODO
    const QString fn = QFileDialog::getSaveFileName(myApp.frame, msg, QString(), pat);
    if (fn.isEmpty()) return tr("Export cancelled.");
    return exportFile(fn, k, true);
}

bool Document::closeDocument()
{
    bool keep = checkForChanges();
    if (!keep && !filename.isEmpty() && QFile::exists(Tools::tmpName(filename)))
    {
        QFile::remove(Tools::tmpName(filename));
    }
    return keep;
}

bool Document::checkForChanges()
{
    if (!modified) return false;
    int ret = QMessageBox::question(
                myApp.frame, filename,
                tr("Changes have been made, are you sure you wish to continue?"),
                tr("Save and Close"),
                tr("Discard Changes"),
                tr("Cancel"), 2);
    if (ret == 0)
    {
        bool success = false;
        save(false, &success);
        return !success;
    }
    if (ret == 1) return false;
    return true;
}

QString Document::action(QPainter &dc, int k)
{
#define RetEmpty return QString()
    shiftToCenter(dc);
    switch (k)
    {
    case A_RUN:
        // TODO
//        myApp.ev.Eval(rootgrid);
//        rootgrid->resetChildren();
//        clearSelectionRefresh();
        return tr("Evaluation finished.");

    case A_UNDO:
        if (undolist.size())
        {
            undo(dc, undolist, redolist);
            return QString();
        }
        else
        {
            return tr("Nothing more to undo.");
        }

    case A_REDO:
        if (redolist.size())
        {
            undo(dc, redolist, undolist, true);
            return nullptr;
        }
        else
        {
            return tr("Nothing more to redo.");
        }

    case A_SAVE: return save(false);
    case A_SAVEAS: return save(true);

    case A_EXPXML: return Export("xml", "*.xml", tr("Choose XML file to write"), k);
    case A_EXPHTMLT:
    case A_EXPHTMLB:
    case A_EXPHTMLO: return Export("html", "*.html", tr("Choose HTML file to write"), k);
    case A_EXPTEXT: return Export("txt", "*.txt", tr("Choose Text file to write"), k);
    case A_EXPIMAGE: return Export("png", "*.png", tr("Choose PNG file to write"), k);
    case A_EXPCSV: return Export("csv", "*.csv", tr("Choose CSV file to write"), k);

    case A_IMPXML:
    case A_IMPXMLA:
    case A_IMPTXTI:
    case A_IMPTXTC:
    case A_IMPTXTS:
    case A_IMPTXTT: return myApp.import(k);

    case A_OPEN: {
        QString fn = QString(); // TODO
//                ::wxFileSelector(_(L"Please select a TreeSheets file to load:"), L"", L"",
//                                 L"cts", L"TreeSheets Files (*.cts)|*.cts|All Files (*.*)|*.*",
//                                 wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
        return myApp.open(fn);
    }

    case A_CLOSE:
    {
        if (myApp.frame->nb->count() <= 1)
        {
            myApp.frame->fromclosebox = false;
            myApp.frame->close();
            RetEmpty;
        }

        if (!closeDocument())
        {
            int p = myApp.frame->nb->currentIndex();
            auto wid = myApp.frame->nb->currentWidget();
            myApp.frame->nb->removeTab(p);
            wid->deleteLater();
            myApp.fhistory->rememberOpenFiles();
        }
        RetEmpty;
    }

    case A_NEW: {
        bool ok;
        int size = QInputDialog::getInt(
                    myApp.frame, tr("New Sheet"),
                    tr("What size grid would you like to start with?\nsize:"), 10, 1, 25, 1, &ok);
        if (!ok || size < 0) return tr("New file cancelled.");
        myApp.initDB(size);
        myApp.frame->getCurTab()->doc->refresh();
        RetEmpty;
    }

    case A_ABOUT: {
        QStringList ls;
        ls << tr("Cover: TreeSheets (https://github.com/aardappel/treesheets)");
        ls << tr("(C) 2009 Wouter van Oortmerssen");
        ls << tr("(C) 2020 Nuyuiq");
        ls << tr("The Free Form Hierarchical Information Organizer");
        QMessageBox::about(myApp.frame, tr("TreeSheets"), ls.join('\n'));
        RetEmpty;
    }

    case A_ABOUTQT: QMessageBox::aboutQt(myApp.frame); RetEmpty;

    case A_HELPI: MyApp::loadTut(); RetEmpty;

    case A_HELP_OP_REF: MyApp::loadOpRef(); RetEmpty;

    case A_HELP:
#ifdef Q_OS_MACOS
        QDesktopServices::openUrl(QUrl("file://" + Tools::resolvePath(QStringLiteral("docs/tutorial.html"), false)));
#else
        QDesktopServices::openUrl(QUrl(Tools::resolvePath(QStringLiteral("docs/tutorial.html"), false)));
#endif
        RetEmpty;

    case A_ZOOMIN: return wheel(dc, 1, false, true, false);  // Zoom( 1, dc); return "zoomed in (menu)";
    case A_ZOOMOUT: return wheel(dc, -1, false, true, false);  // Zoom(-1, dc); return "zoomed out (menu)";
    case A_INCSIZE: return wheel(dc, 1, false, false, true);
    case A_DECSIZE: return wheel(dc, -1, false, false, true);
    case A_INCWIDTH: return wheel(dc, 1, true, false, false);
    case A_DECWIDTH: return wheel(dc, -1, true, false, false);
    case A_INCWIDTHNH: return wheel(dc, 1, true, false, false, false);
    case A_DECWIDTHNH: return wheel(dc, -1, true, false, false, false);

    case A_DEFFONT: {
        bool ok;
        const QFont font = QFontDialog::getFont(&ok, QFont(myApp.cfg->defaultfont, _g::deftextsize), myApp.frame);
        if (ok)
        {
            myApp.cfg->defaultfont = font.family();
            _g::deftextsize = qMin(20, qMax(10, font.pointSize()));
            myApp.cfg->write("defaultfont", myApp.cfg->defaultfont);
            myApp.cfg->write("defaultfontsize", _g::deftextsize);
            // rootgrid->ResetChildren();
            myApp.frame->tabsReset();  // ResetChildren on all
            refresh();
        }
        RetEmpty;
    }

    case A_PRINT: {
        // TODO
//        wxPrintDialogData printDialogData(printData);
//        wxPrinter printer(&printDialogData);
//        MyPrintout printout(this);
//        if (printer.Print(sys->frame, &printout, true)) {
//            printData = printer.GetPrintDialogData().GetPrintData();
//        }
        RetEmpty;
    }

    case A_PRINTSCALE: {
        bool ok;
        uint v = QInputDialog::getInt(myApp.frame, tr("Set Print Scale"),
                    tr("How many pixels wide should a page be? (0 for auto fit)\nscale:"),
                    0, 0, 5000, 1, &ok);
        if (ok) printscale = v;
        RetEmpty;
    }

    case A_PREVIEW: {
        //TODO
//        wxPrintDialogData printDialogData(printData);
//        wxPrintPreview *preview = new wxPrintPreview(
//                    new MyPrintout(this), new MyPrintout(this), &printDialogData);
//        wxPreviewFrame *pframe = new wxPreviewFrame(preview, sys->frame, _(L"Print Preview"),
//                                                    wxPoint(100, 100), wxSize(600, 650));
//        pframe->Centre(wxBOTH);
//        pframe->Initialize();
//        pframe->Show(true);
        RetEmpty;
    }

    case A_PAGESETUP: {
        // TODO
//        pageSetupData = printData;
//        wxPageSetupDialog pageSetupDialog(sys->frame, &pageSetupData);
//        pageSetupDialog.ShowModal();
//        printData = pageSetupDialog.GetPageSetupDialogData().GetPrintData();
//        pageSetupData = pageSetupDialog.GetPageSetupDialogData();
        RetEmpty;
    }

    case A_NEXTFILE: myApp.frame->cycleTabs(1); RetEmpty;
    case A_PREVFILE: myApp.frame->cycleTabs(-1); RetEmpty;

    case A_CUSTCOL: {
        uint c = pickColor(myApp.frame, myApp.cfg->customcolor);
        if (c != (uint)-1) myApp.cfg->customcolor = c;
        RetEmpty;
    }

    case A_DEFBGCOL: {
        uint oldbg = background();
        uint c = pickColor(myApp.frame, oldbg);
        if (c != (uint)-1)
        {
            rootgrid->addUndo(this);
            loopallcells(lc)
            {
                if (lc->cellcolor == oldbg && (!lc->p || lc->p->cellcolor == c))
                    lc->cellcolor = c;
            }
            refresh();
        }
        RetEmpty;
    }

    case A_SEARCHNEXT: return searchNext(dc);

    case A_ROUND0:
    case A_ROUND1:
    case A_ROUND2:
    case A_ROUND3:
    case A_ROUND4:
    case A_ROUND5:
    case A_ROUND6:
        myApp.cfg->roundness = k - A_ROUND0;
        myApp.cfg->write(QString::fromLatin1("roundness"), myApp.cfg->roundness);
        refresh();
        RetEmpty;

    case A_OPENCELLCOLOR:
        if (myApp.frame->celldd) myApp.frame->celldd->showPopup();
        RetEmpty;
    case A_OPENTEXTCOLOR:
        if (myApp.frame->textdd) myApp.frame->textdd->showPopup();
        RetEmpty;
    case A_OPENBORDCOLOR:
        if (myApp.frame->borddd) myApp.frame->borddd->showPopup();
        RetEmpty;

    case A_REPLACEALL: {
        if (myApp.frame->searchstring.isEmpty()) return tr("No search.");
        rootgrid->addUndo(this);  // expensive?
        rootgrid->findReplaceAll(myApp.frame->replaces->text());
        rootgrid->resetChildren();
        refresh();
        RetEmpty;
    }

    case A_SCALED:
        scaledviewingmode = !scaledviewingmode;
        rootgrid->resetChildren();
        refresh();
        return scaledviewingmode ? tr("Now viewing TreeSheet to fit to the screen exactly, "
                                     "press F12 to return to normal.")
                                 : tr("1:1 scale restored.");

    case A_FILTER5:
        editfilter = 5;
        applyEditFilter();
        RetEmpty;
    case A_FILTER10:
        editfilter = 10;
        applyEditFilter();
        RetEmpty;
    case A_FILTER20:
        editfilter = 20;
        applyEditFilter();
        RetEmpty;
    case A_FILTER50:
        editfilter = 50;
        applyEditFilter();
        RetEmpty;
    case A_FILTERM:
        editfilter++;
        applyEditFilter();
        RetEmpty;
    case A_FILTERL:
        editfilter--;
        applyEditFilter();
        RetEmpty;
    case A_FILTERS: setSearchFilter(true); RetEmpty;
    case A_FILTEROFF: setSearchFilter(false); RetEmpty;

    case A_CUSTKEY: {
        // TODO
//        wxArrayString strs;
//        MyFrame::MenuString &ms = sys->frame->menustrings;
//        for (MyFrame::MenuStringIterator it = ms.begin(); it != ms.end(); ++it)
//            strs.push_back(it->first);
//        strs.Sort(CompareMenuString);
//        wxSingleChoiceDialog choice(
//                    sys->frame, _(L"Please pick a menu item to change the key binding for"),
//                    _(L"Key binding"), strs);
//        choice.SetSize(wxSize(500, 700));
//        choice.Centre();
//        if (choice.ShowModal() == wxID_OK) {
//            int sel = choice.GetSelection();
//            wxTextEntryDialog textentry(sys->frame,
//                                        "Please enter the new key binding string",
//                                        "Key binding", ms[sel].second);
//            if (textentry.ShowModal() == wxID_OK) {
//                wxString key = textentry.GetValue();
//                ms[sel].second = key;

//                sys->cfg->Write(ms[sel].first, key);
//                return _(L"NOTE: key binding will take effect next run of TreeSheets.");
//            }
//        }
        return tr("Keybinding cancelled.");
    }
    }

    if (!selected.g) return noSel();

    Cell *c = selected.getCell();

    switch (k) {
    case A_BACKSPACE:
        if (selected.thin())
        {
            if (selected.xs)
                delRowCol(selected.y, 0, selected.g->ys, 1, -1, selected.y - 1, 0, -1);
            else
                delRowCol(selected.x, 0, selected.g->xs, 1, selected.x - 1, -1, -1, 0);
        }
        else if (c && selected.textEdit())
        {
            if (selected.cursorend == 0) return nullptr;
            c->addUndo(this);
            c->text.backspace(selected);
            refresh();
        }
        else selected.g->multiCellDelete(this, selected);
        zoomOutIfNoGrid(dc);
        RetEmpty;

    case A_DELETE:
        if (selected.thin())
        {
            if (selected.xs)
                delRowCol(selected.y, selected.g->ys, selected.g->ys, 0, -1, selected.y, 0,
                          -1);
            else
                delRowCol(selected.x, selected.g->xs, selected.g->xs, 0, selected.x, -1, -1,
                          0);
        }
        else if (c && selected.textEdit())
        {
            if (selected.cursor == c->text.t.length()) RetEmpty;
            c->addUndo(this);
            c->text.Delete(selected);
            refresh();
        }
        else selected.g->multiCellDelete(this, selected);
        zoomOutIfNoGrid(dc);
        RetEmpty;

    case A_DELETE_WORD:
        if (c && selected.textEdit())
        {
            if (selected.cursor == c->text.t.length()) RetEmpty;
            c->addUndo(this);
            c->text.deleteWord(selected);
            refresh();
        }
        zoomOutIfNoGrid(dc);
        RetEmpty;

    case A_COPYCT:
    case A_CUT:
    case A_COPY:
        myApp.frame->clipboardcopy.clear();
        if (selected.thin()) return noThin();

        if (selected.textEdit())
        {
            if (selected.cursor == selected.cursorend)
            {
                return tr("No text selected.");
            }
            myApp.frame->cellclipboard.reset();
        }
        else if (k != A_COPYCT)
        {
            myApp.frame->cellclipboard.reset(c ? c->clone(nullptr) : selected.g->cloneSel(selected));
        }

        if (QClipboard *clipboard = QGuiApplication::clipboard())
        {
            QString s;
            if (k == A_COPYCT)
            {
                loopallcellssel(c, true) if (c->text.t.length()) s += c->text.t + " ";
            }
            else
            {
                s = selected.g->convertToText(selected, 0, A_EXPTEXT, this);
            }
            myApp.frame->clipboardcopy = s;
            clipboard->setText(s);
        }

        if (k == A_CUT)
        {
            if (!selected.textEdit())
            {
                selected.g->cell->addUndo(this);
                selected.g->multiCellDelete(this, selected);
            }
            else if (c)
            {
                c->addUndo(this);
                c->text.backspace(selected);
            }
            refresh();
        }
        zoomOutIfNoGrid(dc);
        RetEmpty;

    case A_SELALL:
        selected.selAll();
        refresh();
        RetEmpty;

    case A_UP:
    case A_DOWN:
    case A_LEFT:
    case A_RIGHT:
        selected.Cursor(this, k, false, false, dc);
        RetEmpty;

    case A_MUP:
    case A_MDOWN:
    case A_MLEFT:
    case A_MRIGHT:
        selected.Cursor(this, k - A_MUP + A_UP, true, false, dc);
        RetEmpty;

    case A_SUP:
    case A_SDOWN:
    case A_SLEFT:
    case A_SRIGHT:
        selected.Cursor(this, k - A_SUP + A_UP, false, true, dc);
        RetEmpty;

    case A_SCLEFT:
    case A_SCRIGHT:
        selected.Cursor(this, k - A_SCUP + A_UP, true, true, dc);
        RetEmpty;

    case A_BOLD:
        selected.g->setStyle(this, selected, STYLE_BOLD);
        RetEmpty;
    case A_ITALIC:
        selected.g->setStyle(this, selected, STYLE_ITALIC);
        RetEmpty;
    case A_TT:
        selected.g->setStyle(this, selected, STYLE_FIXED);
        RetEmpty;
    case A_UNDERL:
        selected.g->setStyle(this, selected, STYLE_UNDERLINE);
        RetEmpty;
    case A_STRIKET:
        selected.g->setStyle(this, selected, STYLE_STRIKETHRU);
        RetEmpty;

    case A_MARKDATA:
    case A_MARKVARD:
    case A_MARKVARU:
    case A_MARKVIEWH:
    case A_MARKVIEWV:
    case A_MARKCODE: {
        // TODO
//        int newcelltype;
//        switch (k) {
//        case A_MARKDATA: newcelltype = CT_DATA; break;
//        case A_MARKVARD: newcelltype = CT_VARD; break;
//        case A_MARKVARU: newcelltype = CT_VARU; break;
//        case A_MARKVIEWH: newcelltype = CT_VIEWH; break;
//        case A_MARKVIEWV: newcelltype = CT_VIEWV; break;
//        case A_MARKCODE: newcelltype = CT_CODE; break;
//        }
//        selected.g->cell->addUndo(this);
//        loopallcellssel(c, false) {
//            c->celltype =
//                    (newcelltype == CT_CODE) ? sys->ev.InferCellType(c->text) : newcelltype;
//            refresh();
//        }
        RetEmpty;
    }

    case A_CANCELEDIT:
        if (selected.textEdit()) break; // TODO break?
        if (selected.g->cell->p)
        {
            selected = selected.g->cell->p->grid->findCell(selected.g->cell);
        }
        else
        {
            selected.selAll();
        }
        scrollOrZoom(dc);
        RetEmpty;

    case A_NEWGRID:
        if (!(c = selected.thinExpand(this))) return oneCell();
        if (c->grid)
        {
            selected = Selection(c->grid, 0, c->grid->ys, 1, 0);
            scrollOrZoom(dc, true);
        }
        else
        {
            c->addUndo(this);
            c->addGrid();
            selected = Selection(c->grid, 0, 0, 1, 1);
            drawSelectMove(dc, selected, true);
        }
        RetEmpty;

    case A_PASTE:
//        if (!(c = selected.thinExpand(this))) return oneCell();
//        if (QClipboard *clipboard = QGuiApplication::clipboard())
//        {
//            wxTheClipboard->GetData(*dataobjc);
//            PasteOrDrop();
//            wxTheClipboard->Close();
//        }
//        else if (myApp.frame->cellclipboard.data())
//        {
//            c->paste(this, myApp.frame->cellclipboard.get(), selected);
//            refresh();
//        }
        RetEmpty;

    case A_PASTESTYLE:
        if (myApp.frame->cellclipboard.isNull()) return tr("No style to paste.");
        selected.g->cell->addUndo(this);
        selected.g->setStyles(selected, myApp.frame->cellclipboard.data());
        selected.g->cell->resetChildren();
        refresh();
        RetEmpty;

    case A_ENTERCELL:
    case A_PROGRESSCELL: {
        if (!(c = selected.thinExpand(this))) return oneCell();
        if (selected.textEdit())
        {
            selected.Cursor(this, (k==A_ENTERCELL ? A_DOWN : A_RIGHT), false, false, dc, true);
        }
        else
        {
            selected.enterEdit(this, 0, c->text.t.length());
            drawSelectMove(dc, selected, true);
        }
        RetEmpty;
    }

    case A_IMAGE: {
//        if (!(c = selected.thinExpand(this))) return oneCell();
//        wxString fn =
//                ::wxFileSelector(_(L"Please select an image file:"), L"", L"", L"", L"*.*",
//                                 wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
//        c->addUndo(this);
//        LoadImageIntoCell(fn, c, sys->frame->csf);
//        refresh();
        RetEmpty;
    }

    case A_IMAGER: {
        selected.g->cell->addUndo(this);
        selected.g->clearImages(selected);
        selected.g->cell->resetChildren();
        refresh();
        RetEmpty;
    }

    case A_SORTD: return sort(true);
    case A_SORT: return sort(false);

    case A_REPLACEONCE:
    case A_REPLACEONCEJ: {
        if (myApp.frame->searchstring.isEmpty()) return tr("No search.");
        selected.g->replaceStr(this, myApp.frame->replaces->text(), selected);
        if (k == A_REPLACEONCEJ) return searchNext(dc);
        RetEmpty;
    }

    case A_SCOLS:
        selected.y = 0;
        selected.ys = selected.g->ys;
        refresh();
        RetEmpty;

    case A_SROWS:
        selected.x = 0;
        selected.xs = selected.g->xs;
        refresh();
        RetEmpty;

    case A_BORD0:
    case A_BORD1:
    case A_BORD2:
    case A_BORD3:
    case A_BORD4:
    case A_BORD5:
        selected.g->cell->addUndo(this);
        selected.g->setBorder(k - A_BORD0 + 1, selected);
        selected.g->cell->resetChildren();
        refresh();
        RetEmpty;

    case A_TEXTGRID: return layrender(-1, true, true);

    case A_V_GS: return layrender(DS_GRID, true);
    case A_V_BS: return layrender(DS_BLOBSHIER, true);
    case A_V_LS: return layrender(DS_BLOBLINE, true);
    case A_H_GS: return layrender(DS_GRID, false);
    case A_H_BS: return layrender(DS_BLOBSHIER, false);
    case A_H_LS: return layrender(DS_BLOBLINE, false);
    case A_GS: return layrender(DS_GRID, true, false, true);
    case A_BS: return layrender(DS_BLOBSHIER, true, false, true);
    case A_LS: return layrender(DS_BLOBLINE, true, false, true);

    case A_WRAP: return selected.wrap(this);

    case A_RESETSIZE:
    case A_RESETWIDTH:
    case A_RESETSTYLE:
    case A_RESETCOLOR:
    case A_LASTCELLCOLOR:
    case A_LASTTEXTCOLOR:
    case A_LASTBORDCOLOR:
        selected.g->cell->addUndo(this);
        loopallcellssel(c, true)
        {
            switch (k) {
            case A_RESETSIZE:
                c->text.relsize = 0;
                break;
            case A_RESETWIDTH:
                if (c->grid) c->grid->initColWidths();
                break;
            case A_RESETSTYLE:
                c->text.stylebits = 0;
                break;
            case A_RESETCOLOR:
                c->cellcolor = 0xFFFFFF;
                c->textcolor = 0;
                if (c->grid) c->grid->bordercolor = 0xA0A0A0;
                break;
            case A_LASTCELLCOLOR:
                c->cellcolor = myApp.cfg->lastcellcolor;
                break;
            case A_LASTTEXTCOLOR:
                c->textcolor = myApp.cfg->lasttextcolor;
                break;
            case A_LASTBORDCOLOR:
                if (c->grid) c->grid->bordercolor = myApp.cfg->lastbordcolor;
                break;
            }
        }
        selected.g->cell->resetChildren();
        refresh();
        RetEmpty;

    case A_MINISIZE: {
        selected.g->cell->addUndo(this);
        collectCellsSel(false);
        QVector<Cell *> outer;
        outer.append(itercells);
        for (int i = 0; i < outer.size(); i++)
        {
            Cell *o = outer[i];
            if (o->grid)
            {
                loopcellsin(o, c) if (_i)
                {
                    c->text.relsize = _g::deftextsize - _g::mintextsize() - c->depth();
                }
            }
        }
        outer.resize(0);
        selected.g->cell->resetChildren();
        refresh();
        RetEmpty;
    }

    case A_FOLD:
    case A_FOLDALL:
    case A_UNFOLDALL:
        loopallcellssel(c, k != A_FOLD) if (c->grid) {
            c->addUndo(this);
            c->grid->folded = k == A_FOLD ? !c->grid->folded : k == A_FOLDALL;
            c->resetChildren();
        }
        refresh();
        RetEmpty;

    case A_HOME:
    case A_END:
    case A_CHOME:
    case A_CEND:
        if (selected.textEdit()) break;
        selected.homeEnd(this, dc, k == A_HOME || k == A_CHOME);
        RetEmpty;

    case A_IMAGESCN: {
        // TODO
//        loopallcellssel(c, true) if (c->text.image)
//        {
//                c->text.image->resetScale(myApp.frame->csf);
//        }
//        selected.g->cell->resetChildren();
//        refresh();
        RetEmpty;
    }
    }

    if (c || (!c && selected.isAll()))
    {
        Cell *ac = c ? c : selected.g->cell;
        switch (k) {
        case A_TRANSPOSE:
            if (ac->grid)
            {
                ac->addUndo(this);
                ac->grid->transpose();
                ac->resetChildren();
                selected = ac->p ? ac->p->grid->findCell(ac) : Selection();
                refresh();
                RetEmpty;
            }
            else return noGrid();

        case A_HIFY:
            if (!ac->grid) return noGrid();
            if (!ac->grid->isTable())
                return tr("Selected grid is not a table: cells must not already have "
                         "sub-grids.");
            ac->addUndo(this);
            ac->grid->hierarchify(this);
            ac->resetChildren();
            clearSelectionRefresh();
            RetEmpty;

        case A_FLATTEN: {
            if (!ac->grid) return noGrid();
            ac->addUndo(this);
            int maxdepth = 0, leaves = 0;
            ac->maxDepthLeaves(0, maxdepth, leaves);
            Grid *g = new Grid(maxdepth, leaves);
            g->initCells();
            ac->grid->flatten(0, 0, g);
            DELPTR(ac->grid);
            ac->grid = g;
            g->reParent(ac);
            ac->resetChildren();
            clearSelectionRefresh();
            RetEmpty;
        }
        }
    }

    if (!c) return oneCell();

    switch (k) {
    case A_NEXT: selected.next(this, dc, false); RetEmpty;
    case A_PREV: selected.next(this, dc, true); RetEmpty;

    case A_BROWSE:
        if (!QDesktopServices::openUrl(QUrl(c->text.toText(0, selected, A_EXPTEXT))))
            return tr("Cannot launch browser for this link.");
        RetEmpty;

    case A_BROWSEF: {
        // TODO
//        QString f = c->text.toText(0, selected, A_EXPTEXT);
//        QFileInfo fn(f);
//        if (fn.isRelative()) fn.makeAbsolute(QFileInfo(filename).filePath());
//        if (!wxLaunchDefaultApplication(fn.GetFullPath())) return _(L"Cannot find file.");
        RetEmpty;
    }

    case A_IMAGESCP:
    case A_IMAGESCF: {
        // TODO
//        if (!c->text.image) return _(L"No image in this cell.");
//        long v = wxGetNumberFromUser(
//                    _(L"Please enter the percentage you want the image scaled by:"), L"%",
//                    _(L"Image Resize"), 50, 5, 400, sys->frame);
//        if (v < 0) return nullptr;
//        auto sc = v / 100.0;
//        if (k == A_IMAGESCP) {
//            c->text.image->BitmapScale(sc);
//        } else {
//            c->text.image->DisplayScale(sc);
//        }
//        c->ResetLayout();
//        Refresh();
        RetEmpty;
    }

    case A_ENTERGRID:
        if (!c->grid) action(dc, A_NEWGRID);
        selected = Selection(c->grid, 0, 0, 1, 1);
        scrollOrZoom(dc, true);
        RetEmpty;

    case A_LINK:
    case A_LINKREV: {
        if (c->text.t.isEmpty()) return tr("No text in this cell.");
        bool t1 = false, t2 = false;
        Cell *link = rootgrid->findLink(selected, c, nullptr, t1, t2, k == A_LINK);
        if (!link || !link->p) return tr("No matching cell found!");
        selected = link->p->grid->findCell(link);
        scrollOrZoom(dc, true);
        RetEmpty;
    }

    case A_COLCELL: myApp.cfg->customcolor = c->cellcolor; RetEmpty;

    case A_HSWAP: {
        Cell *pp = c->p->p;
        if (!pp) return tr("Cannot move this cell up in the hierarchy.");
        if (pp->grid->xs != 1 && pp->grid->ys != 1)
            return tr("Can only move this cell into a Nx1 or 1xN grid.");
        if (c->p->grid->xs != 1 && c->p->grid->ys != 1)
            return tr("Can only move this cell from a Nx1 or 1xN grid.");
        pp->addUndo(this);
        selected = pp->grid->hierarchySwap(c->text.t);
        pp->resetChildren();
        pp->resetLayout();
        refresh();
        RetEmpty;
    }

    case A_TAGADD:
        if (c->text.t.isEmpty()) return tr("Empty strings cannot be tags.");
        if (!tags.contains(c->text.t)) tags << c->text.t;
        refresh();
        RetEmpty;

    case A_TAGREMOVE:
        // TODO
        tags.removeAll(c->text.t);
        // tags.erase(c->text.t);
        refresh();
        RetEmpty;
    }

    if (!selected.textEdit()) return tr("only works in cell text mode");

    switch (k) {
    case A_CANCELEDIT:
        if (lastUndoSameCell(c))
            undo(dc, undolist, redolist);
        else
            refresh();
        selected.exitEdit(this);
        RetEmpty;

    case A_BACKSPACE_WORD:
        if (selected.cursorend == 0) return nullptr;
        c->addUndo(this);
        c->text.backspaceWord(selected);
        refresh();
        zoomOutIfNoGrid(dc);
        RetEmpty;

        // FIXME: this functionality is really SCHOME, SHOME should be within line
    case A_SHOME:
        drawSelect(dc, selected);
        selected.cursor = 0;
        drawSelectMove(dc, selected);
        RetEmpty;
    case A_SEND:
        drawSelect(dc, selected);
        selected.cursorend = c->text.t.length();
        drawSelectMove(dc, selected);
        RetEmpty;

    case A_CHOME:
        drawSelect(dc, selected);
        selected.cursor = selected.cursorend = 0;
        drawSelectMove(dc, selected);
        RetEmpty;
    case A_CEND:
        drawSelect(dc, selected);
        selected.cursor = selected.cursorend = selected.maxCursor();
        drawSelectMove(dc, selected);
        RetEmpty;

    case A_HOME:
        drawSelect(dc, selected);
        c->text.homeEnd(selected, true);
        drawSelectMove(dc, selected);
        RetEmpty;
    case A_END:
        drawSelect(dc, selected);
        c->text.homeEnd(selected, false);
        drawSelectMove(dc, selected);
        RetEmpty;
    default: return tr("Internal error: unimplemented operation!");
    }
}

QString Document::tagSet(int tagno)
{
    Q_ASSERT(tagno >= 0 && tagno < tags.size());
    Cell *c = selected.getCell();
    if (!c) return oneCell();
    c->addUndo(this);
    c->text.clear(this, selected);
    c->text.insert(this, tags.at(tagno), selected);
    refresh();
    return QString();
}

void Document::undo(QPainter &dc, QList<UndoItem *> &fromlist, QList<UndoItem *> &tolist, bool redo)
{
    // TODO
//    Selection beforesel = selected;
//    QList<Selection> beforepath; // TODO beforepath type
//    if (beforesel.g) createPath(beforesel.g->cell, beforepath);
//    UndoItem *ui = fromlist.takeLast();
//    Cell *c = walkPath(ui->path);
//    auto clone = ui->clone;
//    ui->clone = c;
//    if (c->p && c->p->grid)
//    {
//        c->p->grid->replaceCell(c, clone);
//        clone->p = c->p;
//    }
//    else rootgrid = clone;
//    clone->resetLayout();
//    selected = ui->sel;
//    if (selected.g) selected.g = walkPath(ui->selpath)->grid;
//    begindrag = selected;
//    ui->sel = beforesel;
//    ui->selpath.clear();
//    ui->selpath.append(beforepath); // TODO
//    tolist.append(ui);
//    if (undolistsizeatfullsave > undolist.size())
//        undolistsizeatfullsave = -1;  // gone beyond the save point, always modified
//    modified = undolistsizeatfullsave != undolist.size();
//    if (selected.g) scrollOrZoom(dc);
//    else refresh();
//    updateFileName();
}

void Document::scrollOrZoom(QPainter &dc, bool zoomiftiny)
{
    if (!selected.g) return;
    Cell *drawroot = walkPath(drawpath);
    // If we jumped to a cell which may be insided a folded cell, we have to unfold it
    // because the rest of the code doesn't deal with a selection that is invisible :)
    for (Cell *cg = selected.g->cell; cg; cg = cg->p)
    {
        // Unless we're under the drawroot, no need to unfold further.
        if (cg == drawroot) break;
        if (cg->grid->folded)
        {
            cg->grid->folded = false;
            cg->resetLayout();
            cg->resetChildren();
        }
    }
    for (Cell *cg = selected.g->cell; cg; cg = cg->p)
    {
        if (cg == drawroot)
        {
            if (zoomiftiny) zoomTiny(dc);
            drawSelectMove(dc, selected, true);
            return;
        }
    }
    zoom(-100, dc, false, false);
    if (zoomiftiny) zoomTiny(dc);
    drawSelectMove(dc, selected, true);
}

void Document::zoomTiny(QPainter &dc)
{
    Cell *c = selected.getCell();
    if (c && c->tiny)
    {
        int rels = c->text.relsize;
        while (fontIsMini(textSize(c->depth(), rels))) rels--;
        zoom(c->text.relsize - rels, dc);  // seems to leave selection box in a weird location?
    }
}

uint Document::pickColor(QWidget *fr, uint defcol)
{
    auto col = QColorDialog::getColor(Color(defcol), fr);
    return col.isValid()? Color(col).toBGR(): -1;
}

void Document::collectCells(Cell *c)
{
    itercells.resize(0);
    c->collectCells(itercells);
}

void Document::collectCellsSel(bool recurse)
{
    itercells.resize(0);
    if (selected.g) selected.g->collectCellsSel(itercells, selected, recurse);
}

QString Document::searchNext(QPainter &dc)
{
    if (myApp.frame->searchstring.isEmpty()) return tr("No search string.");
    bool lastsel = true;
    Cell *next =
            rootgrid->findNextSearchMatch(myApp.frame->searchstring, nullptr, selected.getCell(), lastsel);
    if (!next) return tr("No matches for search.");
    selected = next->p->grid->findCell(next);
    sw->setFocus();
    scrollOrZoom(dc, true);
    return QString();
}

static int _timesort(const Cell *a, const Cell *b) {
    return (a->text.lastedit < b->text.lastedit) * 2 - 1;
}

void Document::applyEditFilter()
{
    searchfilter = false;
    editfilter = qMin(qMax(editfilter, 1), 99);
    collectCells(rootgrid);
    qSort(itercells.begin(), itercells.end(), _timesort);
    for (int i = 0; i < itercells.size(); i++)
    {
        itercells[i]->text.filtered = i > itercells.size() * editfilter / 100;
    }
    rootgrid->resetChildren();
    refresh();
}

void Document::setSearchFilter(bool on)
{
    searchfilter = on;
    loopallcells(c) c->text.filtered = on && !c->text.isInSearch();
    rootgrid->resetChildren();
    refresh();
}

void Document::delRowCol(int &v, int e, int gvs, int dec, int dx, int dy, int nxs, int nys)
{
    if (v != e)
    {
        selected.g->cell->addUndo(this);
        if (gvs == 1)
        {
            selected.g->delSelf(this, selected);
        }
        else
        {
            selected.g->deleteCells(dx, dy, nxs, nys);
            v -= dec;
        }
        refresh();
    }
}

void Document::zoomOutIfNoGrid(QPainter &dc)
{
    if (!walkPath(drawpath)->grid) zoom(-1, dc);
}

QString Document::sort(bool descending)
{
    if (selected.xs != 1 && selected.ys <= 1)
        return tr("Can't sort: make a 1xN selection to indicate what column to sort on, and "
                 "what rows to affect");
    selected.g->cell->addUndo(this);
    selected.g->sort(selected, descending);
    refresh();
    RetEmpty;
}

QString Document::layrender(int ds, bool vert, bool toggle, bool noset)
{
    if (selected.thin()) return noThin();
    selected.g->cell->addUndo(this);
    bool v = toggle ? !selected.getFirst()->verticaltextandgrid : vert;
    if (ds >= 0 && selected.isAll()) selected.g->cell->drawstyle = ds;
    selected.g->setGridTextLayout(ds, v, noset, selected);
    selected.g->cell->resetChildren();
    refresh();
    return nullptr;
}

UndoItem::~UndoItem()
{
    DELPTR(clone);
}
