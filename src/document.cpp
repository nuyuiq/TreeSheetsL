#include "document.h"
#include "grid.h"
#include "tools.h"
#include "myapp.h"
#include "mainwindow.h"
#include "cell.h"
#include "widget.h"
#include "config.h"
#include "widget.h"
#include "symdef.h"

#include <QBuffer>
#include <QFileInfo>
#include <QPainter>
#include <QDebug>
#include <QScrollBar>
#include <QScrollArea>
#include <QMessageBox>
#include <QColorDialog>
#include <QMimeData>
#include <QUrl>
#include <Qprinter>


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

    // 备份有效区域
    int bak[] = {originx, originy, maxx, maxy};
    // 裁剪绘制区
    auto roi = dc.clipBoundingRect().toRect();
    if (roi.width() == 0 || roi.height() == 0) return;
    else
    {
        const auto &o = QRect(originx, originy, maxx - originx, maxy - originy);
        const auto &tf = QTransform()
                .scale(1/currentviewscale, 1/currentviewscale)
                .translate(-centerx, -centery);
        const auto &n = o & tf.mapRect(roi);
        n.getCoords(&originx, &originy, &maxx, &maxy);
    }

    shiftToCenter(dc);

    render(dc);
    drawSelect(dc, selected);
    if (hover.g) hover.g->drawHover(this, dc, hover);
    if (scaledviewingmode) { dc.scale(1, 1); }

    // 还原有效区域
    originx = bak[0];
    originy = bak[1];
    maxx    = bak[2];
    maxy    = bak[3];
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
    if (LastUndoSameCellTextEdit(c)) return;
    UndoItem *ui = new UndoItem();
    undolist.append(ui);
    ui->clone = c->clone(nullptr);
    ui->sel = selected;
    ui->cloned_from = (uintptr_t)c;
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
            if (!lastUndoSameCellAny(selected.g->cell)) selected.g->cell->addUndo(this);
            selected.g->resizeColWidths(dir, selected, hierarchical);
            selected.g->cell->resetLayout();
            selected.g->cell->resetChildren();
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
        dir = dir > 0? 1: -1;
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
    sw->update();
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
}

bool Document::LastUndoSameCellTextEdit(Cell *c) const
{
    // hacky way to detect word boundaries to stop coalescing, but works, and
    // not a big deal if selected is not actually related to this cell
    return undolist.size() && !c->grid && undolist.size() != undolistsizeatfullsave &&
            undolist.last()->sel.eqLoc(c->p->grid->findCell(c)) &&
            (!c->text.t.endsWith(QChar::fromLatin1(' ')) || c->text.t.length() != selected.cursor);
}

bool Document::lastUndoSameCellAny(Cell *c)
{
    return undolist.size() && undolist.size() != undolistsizeatfullsave &&
            undolist.last()->cloned_from == (uintptr_t)c;
}

QString Document::save(bool saveas, bool *success)
{
    if (!saveas && !filename.isEmpty())
    {
        return saveDB(success);
    }
    const QString &fn = Dialog::saveFile(tr("Choose TreeSheets file to save:"),
                tr("TreeSheets Files (*.cts);;All Files (*.*)"));
    if (fn.isEmpty())
    {
        return tr("Save cancelled.");  // avoid name being set to ""
    }
    changeFileName(fn, true);
    return saveDB(success);
}

void Document::autoSave(bool minimized, int page)
{
    if (myApp.cfg->autosave && tmpsavesuccess && !filename.isEmpty() && lastmodsinceautosave)
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
    else if (k == A_EXPIMAGE)
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
    QString fn = Dialog::saveFile(msg, pat);
    if (fn.isEmpty()) return tr("Export cancelled.");
    if (!fn.endsWith(fmt))
    {
        fn += "." + fmt;
    }
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

void Document::undo(QPainter &dc, QList<UndoItem *> &fromlist, QList<UndoItem *> &tolist)
{
    Selection beforesel = selected;
    QVector<Selection> beforepath;
    if (beforesel.g) createPath(beforesel.g->cell, beforepath);
    UndoItem *ui = fromlist.takeLast();
    Cell *c = walkPath(ui->path);
    auto clone = ui->clone;
    ui->clone = c;
    if (c->p && c->p->grid)
    {
        c->p->grid->replaceCell(c, clone);
        clone->p = c->p;
    }
    else rootgrid = clone;
    clone->resetLayout();
    selected = ui->sel;
    if (selected.g) selected.g = walkPath(ui->selpath)->grid;
    begindrag = selected;
    ui->sel = beforesel;
    ui->selpath = beforepath;
    tolist.append(ui);
    if (undolistsizeatfullsave > undolist.size())
        undolistsizeatfullsave = -1;  // gone beyond the save point, always modified
    modified = undolistsizeatfullsave != undolist.size();
    if (selected.g) scrollOrZoom(dc);
    else refresh();
    updateFileName();
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
        zoom(1, dc);  // seems to leave selection box in a weird location?
        if (selected.getCell() != c) zoomTiny(dc);
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
    return QString();
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

void Document::pasteOrDrop(const QMimeData *dataobjc)
{
    Cell *c = selected.getCell();
    if (!(c = selected.thinExpand(this))) return;

    if (dataobjc->hasUrls())
    {
        auto us = dataobjc->urls();
        if (us.size() > 1)
        {
            sw->status(tr("Cannot drag & drop more than 1 file."));
        }
        c->addUndo(this);
        auto fn = us.at(0);
        auto txt = fn.isLocalFile()? fn.toLocalFile(): fn.toString();
        if (!loadImageIntoCell(txt, c, 1))
        {
            pasteSingleText(c, txt);
        }
        refresh();
    }
    else if (dataobjc->hasImage())
    {
        const QPixmap &im = qvariant_cast<QPixmap>(dataobjc->imageData());
        c->addUndo(this);
        setImageBM(c, im.toImage(), 1);
        c->reset();
        refresh();
    }
    else if (dataobjc->hasHtml())
    {
        // TODO
        // Would have to somehow parse HTML here to get images and styled text.
        // Would have to somehow parse RTF here to get images and styled text.
    }
    // several text formats
    else if (dataobjc->hasText())
    {
        const QString s = dataobjc->text();
        if (!s.isEmpty())
        {
            if ((myApp.frame->clipboardcopy == s) && myApp.frame->cellclipboard)
            {
                c->paste(this, myApp.frame->cellclipboard.data(), selected);
                refresh();
            }
            else
            {
                const QStringList &as = s.split(QString::fromLatin1(LINE_SEPERATOR));
                if (as.size())
                {
                    if (as.size() == 1)
                    {
                        c->addUndo(this);
                        pasteSingleText(c, as[0]);
                    }
                    else
                    {
                        c->p->addUndo(this);
                        c->resetLayout();
                        DELPTR(c->grid);
                        Grid::fillRows(c->addGrid(), as, Tools::countCol(as[0]), 0, 0);
                        if (!c->hasText())
                        {
                            c->grid->mergeWithParent(c->p->grid, selected);
                        }
                    }
                    refresh();
                }
            }
        }
    }
}

bool Document::loadImageIntoCell(const QString &fn, Cell *c, double sc)
{
    if (fn.isEmpty()) return false;
    QImage im(fn);
    if (im.isNull()) return false;
    setImageBM(c, im, sc);
    c->reset();
    return true;
}

void Document::setImageBM(Cell *c, const QImage &im, double sc)
{
    c->text.image = myApp.wrapImage(Image(im, sc));
}

void Document::pasteSingleText(Cell *c, const QString &t)
{
    c->text.insert(this, t, selected);
}

QString Document::key(const QString &str, Qt::KeyboardModifiers modifiers)
{
    if (str.size() == 1 && !str.at(0).isPrint())
    {
        int kv = str.at(0).unicode();
        if (kv == '\b')
        {
            Tools::Painter dc(sw);
            return action(dc, A_BACKSPACE);
        }
        if (kv == '\r')
        {
            Tools::Painter dc(sw);
            return action(dc, modifiers & Qt::ShiftModifier ? A_ENTERGRID : A_ENTERCELL);
        }
        qDebug() << "unknown key" << kv;
    }
    else
    {
        if (!selected.g) return noSel();
        Cell *c;
        if (!(c = selected.thinExpand(this))) return oneCell();
        Tools::Painter dc(sw);
        shiftToCenter(dc);
        c->addUndo(this); // FIXME: not needed for all keystrokes, or at least, merge all
        c->text.key(this, str, selected);
        scrollIfSelectionOutOfView(dc, selected, true);
    }
    return QString();
}

void Document::print(QPrinter *p)
{
    QPainter dc(p);
    layout(dc);
    maxx = layoutxs;
    maxy = layoutys;
    originx = originy = 0;
    auto psize = p->pageRect().size();
    auto dsize = QSize(maxx, maxy);
    qreal ds = qMin((qreal)psize.width() / dsize.width(), (qreal)psize.height() / dsize.height());
    dc.scale(ds, ds);
    if (!qFuzzyIsNull(ds)) psize /= ds;
    auto offs = (psize - dsize) / 2;
    dc.translate(offs.width(), offs.height());
    dc.setClipRect(QRect(QPoint(0, 0), dsize));
    while_printing = true;
    render(dc);
    while_printing = false;
}

UndoItem::~UndoItem()
{
    DELPTR(clone);
}
