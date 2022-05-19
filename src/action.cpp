#include "document.h"
#include "grid.h"
#include "cell.h"
#include "history.h"
#include "dropdown.h"
#include "image.h"
#include "config.h"
#include "myapp.h"
#include "tools.h"
#include "mainwindow.h"
#include "widget.h"
#include "symdef.h"

#include <QDesktopServices>
#include <QFontDialog>
#include <QColorDialog>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QProcess>
#include <QUrl>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QDebug>

void MainWindow::actionActivated()
{
    Widget *sw = getCurTab();
    QAction *action = qobject_cast<QAction*>(sender());
    auto Check = [&](const char *cfg, bool *var = nullptr)
    {
        if (action)
        {
            bool b;
            if (!var) var = &b;
            *var = action->isChecked();
            myApp.cfg->write(QString::fromLatin1(cfg), *var);
            sw->status(tr("change will take effect next run of TreeSheets"));
        }
    };
    int kid = sender()->property("kid").toInt();
qDebug() << kid;
    switch (kid)
    {
    case A_NOP: break;
    case A_ALEFT: sw->cursorScroll(-_g::scrollratecursor, 0); break;
    case A_ARIGHT: sw->cursorScroll(_g::scrollratecursor, 0); break;
    case A_AUP: sw->cursorScroll(0, -_g::scrollratecursor); break;
    case A_ADOWN: sw->cursorScroll(0, _g::scrollratecursor); break;

    case A_ICONSET:   Check("iconset"); break;
    case A_SHOWSBAR:  Check("showsbar"); break;
    case A_SHOWTBAR:  Check("showtbar"); break;
    case A_LEFTTABS:  Check("lefttabs"); break;
    case A_SINGLETRAY:Check("singletray"); break;
    case A_MAKEBAKS:  Check("makebaks", &myApp.cfg->makebaks); break;
    case A_TOTRAY:    Check("totray", &myApp.cfg->totray); break;
    case A_MINCLOSE:  Check("minclose", &myApp.cfg->minclose); break;
    case A_ZOOMSCR:   Check("zoomscroll", &myApp.cfg->zoomscroll); break;
    case A_THINSELC:  Check("thinselc", &myApp.cfg->thinselc); break;
    case A_AUTOSAVE:  Check("autosave", &myApp.cfg->autosave); break;
    case A_CENTERED:  Check("centered", &myApp.cfg->centered); update(); break;
    case A_FSWATCH:   Check("fswatch", &myApp.cfg->fswatch); break;
    case A_AUTOEXPORT:Check("autohtmlexport", &myApp.cfg->autohtmlexport); break;
    case A_FASTRENDER:Check("fastrender", &myApp.cfg->fastrender); update(); break;
    case A_FULLSCREEN:
        if (isFullScreen()) showNormal();
        else
        {
            showFullScreen();
            sw->status(tr("Press F11 to exit fullscreen mode."));
        }
        break;
    case A_ZEN_MODE:
        if (!isFullScreen())
        {
            if (tb != nullptr) tb->setVisible(zenmode);
            if (sb != nullptr) sb->setVisible(zenmode);
            this->resize(size()); // TODO
            this->update();
            if (tb != nullptr) tb->update();
            if (sb != nullptr) sb->update();
            zenmode = !zenmode;
        }
        break;
    case A_SEARCH:
    {
        searchstring = filter->text();
        Document *doc = getCurTab()->doc;
        doc->selected.g = nullptr;
        if (doc->searchfilter)
        {
            doc->setSearchFilter(!searchstring.isEmpty());
        }
        else doc->refresh();
        doc->sw->status(QString());
        break;
    }
    case A_SEARCHF:
        if (filter)
        {
            filter->setFocus();
            filter->selectAll();
        }
        else
        {
            sw->status(tr("Please enable (Options -> Show Toolbar) to use search."));
        }
        break;
    case A_EXIT:
        fromclosebox = false;
        myApp.frame->close();
        break;
    case A_CLOSE:
    {
        Tools::Painter dc(sw);
        sw->doc->shiftToCenter(dc);
        sw->doc->action(dc, kid);
        break;  // sw dangling pointer on return
    }
    default:
        if (kid >= A_FILEHIS0 && kid <= A_FILEHIS0 + 8)
        {
            const QString &fn = myApp.fhistory->getHistoryFile(kid - A_FILEHIS0);
            sw->status(myApp.open(fn));
        }
        else if (kid >= A_TAGSET && kid < A_SCRIPT)
        {
            sw->status(sw->doc->tagSet(kid - A_TAGSET));
        }
        else if (kid >= A_SCRIPT && kid < A_MAXACTION)
        {
            const QString &sf = myApp.cfg->scriptsInMenu.at(kid - A_SCRIPT);
            // TODO
            qDebug() << "script run :" << sf;
            sw->status(QString("scrip run(todo)"));
//            auto msg = tssi.ScriptRun(sf);
//            msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
//            sw->status(wxString(msg));
        }
        else
        {
            Tools::Painter dc(sw);
            sw->doc->shiftToCenter(dc);
            sw->status(sw->doc->action(dc, kid));
        }
    }
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
//    case A_IMPXMLA:
    case A_IMPTXTI:
    case A_IMPTXTC:
    case A_IMPTXTS:
    case A_IMPTXTT: return myApp.import(k);

    case A_OPEN: {
        const QString &fn =
                Dialog::openFile(
                    tr("Please select a TreeSheets file to load:"),
                    tr("TreeSheets Files (*.cts);;All Files (*.*)"));
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
        int size = Dialog::intValue(
                    tr("New Sheet"),
                    tr("What size grid would you like to start with?\nsize:"),
                     1, 25, 10);
        if (size < 0) return tr("New file cancelled.");
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
#ifdef Q_OS_MAC
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
        int v = Dialog::intValue(tr("Set Print Scale"),
                    tr("How many pixels wide should a page be? (0 for auto fit)\nscale:"),
                    0, 5000, 0);
        if (v >= 0) printscale = v;
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
        return scaledviewingmode ?
                    tr("Now viewing TreeSheet to fit to the screen exactly, "
                       "press F12 to return to normal."):
                    tr("1:1 scale restored.");

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
        QMap<QString, QString> &ms = myApp.cfg->menuShortcutMap;
        QStringList strs = ms.keys();
        auto compareMS = [](const QString &first, const QString &second) {
            auto a = first.midRef(0);
            auto b = second.midRef(0);

            if (a.at(0) == '&') a = a.mid(1);
            if (b.at(0) == '&') b = b.mid(1);
            return a.compare(b);
        };
        qSort(strs.begin(), strs.end(), compareMS);
        bool ok;
        const QString key = QInputDialog::getItem(
                    myApp.frame,
                    tr("Key binding"),
                    tr("Please pick a menu item to change the key binding for"),
                    strs, 0, false, &ok);
        if (ok && !key.isEmpty())
        {
            QString val = QInputDialog::getText(
                        myApp.frame,
                        tr("Key binding"),
                        tr("Please enter the new key binding string"),
                        QLineEdit::Normal,
                        key,
                        &ok);
            if (ok)
            {
                ms[key] = val;
                myApp.cfg->write(ms[key], val);
                return tr("NOTE: key binding will take effect next run of TreeSheets.");
            }
        }
        return tr("Keybinding cancelled.");
    }
    }

    if (!selected.g) return noSel();

    for (;;)
    {
        uint color;
        if (k == A_CELLCOLOR)
        {
            color = myApp.cfg->lastcellcolor = myApp.frame->celldd->currentColor().toBGR();
        }
        else if (k == A_TEXTCOLOR)
        {
            color = myApp.cfg->lasttextcolor = myApp.frame->textdd->currentColor().toBGR();
        }
        else if (k == A_BORDCOLOR)
        {
            color = myApp.cfg->lastbordcolor = myApp.frame->borddd->currentColor().toBGR();
        }
        else break;
        selected.g->colorChange(this, k, color, selected);
        RetEmpty;
    }

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
            myApp.frame->clipboardcopy = s; // TODO
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
        if (!(c = selected.thinExpand(this))) return oneCell();
        pasteOrDrop(QGuiApplication::clipboard()->mimeData());
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
        if (!(c = selected.thinExpand(this))) return oneCell();
        const QString fn =
                Dialog::openFile(
                    tr("Please select an image file:"),
                    QString::fromLatin1("*.* (*.*)"));
        c->addUndo(this);
        loadImageIntoCell(fn, c, 1); // TODO sys->frame->csf
        refresh();
        RetEmpty;
    }

    case A_IMAGER: {
        selected.g->cell->addUndo(this);
        selected.g->clearImages(selected);
        selected.g->cell->resetChildren();
        refresh();
        RetEmpty;
    }
    case A_DDIMAGE:
        selected.g->cell->addUndo(this);
        loopallcellssel(c, false)
        {
            loadImageIntoCell(myApp.frame->idd->currentText(), c, dd_icon_res_scale);
        }
        refresh();
        RetEmpty;
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
        loopallcellssel(c, k != A_FOLD) if (c->grid)
        {
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
        loopallcellssel(c, true) if (c->text.image)
        {
                c->text.image->resetScale(1); // TODO sys->frame->csf
        }
        selected.g->cell->resetChildren();
        refresh();
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
        {
            return tr("Cannot launch browser for this link.");
        }
        RetEmpty;

    case A_BROWSEF: {
        const QString &f = c->text.toText(0, selected, A_EXPTEXT);
        QFileInfo fn(f);
        if (fn.isRelative())
        {
            fn.setFile(QFileInfo(filename).absolutePath() + "/" + f);
        }
        if (!QProcess::startDetached(fn.absoluteFilePath()))
        {
            return tr("Cannot find file.");
        }
        RetEmpty;
    }

    case A_IMAGESCP:
    case A_IMAGESCF: {
        if (!c->text.image) return tr("No image in this cell.");
        long v = Dialog::intValue(
                    tr("Image Resize"),
                    tr("Please enter the percentage you want the image scaled by(%):"),
                    5, 400, 50);
        if (v < 0) RetEmpty;
        auto sc = v / 100.0;
        if (k == A_IMAGESCP) {
            c->text.image->bitmapScale(sc);
        } else {
            c->text.image->displayScale(sc);
        }
        c->resetLayout();
        refresh();
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
        tags.removeAll(c->text.t);
        refresh();
        RetEmpty;
    }

    if (!selected.textEdit()) return tr("only works in cell text mode");

    switch (k) {
    case A_CANCELEDIT:
        if (lastUndoSameCell(c)) undo(dc, undolist, redolist);
        else refresh();
        selected.exitEdit(this);
        RetEmpty;

    case A_BACKSPACE_WORD:
        if (selected.cursorend == 0) RetEmpty;
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
    default:
        qDebug() << "unimplemented operation!" << k;
        return tr("Internal error: unimplemented operation!");
    }
}
