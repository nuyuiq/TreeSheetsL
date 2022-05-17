#include "mainwindow.h"
#include "widget.h"
#include "tools.h"
#include "symdef.h"
#include "myapp.h"
#include "config.h"
#include "dropdown.h"
#include "history.h"
#include "document.h"
#include "cell.h"
#include "grid.h"

#include <QScrollArea>
#include <QIcon>
#include <QDebug>
#include <QDir>
#include <QMenuBar>
#include <QShortcut>
#include <QToolBar>
#include <QLabel>
#include <QLineEdit>
#include <QStatusBar>
#include <QApplication>
#include <QDesktopWidget>
#include <QFileSystemWatcher>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    myApp.frame = this;
    fromclosebox = true;
    zenmode = false;
    initUI();
    watcher = new QFileSystemWatcher(this);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::fileChanged);

    blinkTimer = startTimer(400);
    savechecker = startTimer(1000);
}

MainWindow::~MainWindow()
{
    if (savechecker) killTimer(savechecker);
    if (blinkTimer) killTimer(blinkTimer);
}

Widget *MainWindow::createWidget(bool append)
{
    QScrollArea*scroll = new QScrollArea;
    Q_ASSERT(scroll);
    Widget* widget = new Widget(scroll);
    Q_ASSERT(widget);
    nb->insertTab(append? -1: 0, scroll, QString());
    widget->resize(scroll->size());
    return widget;
}

// 托盘图标事件
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    bool b = reason == (myApp.cfg->singletray?
                            QSystemTrayIcon::Trigger:
                            QSystemTrayIcon::DoubleClick);
    if (b) deIconize();
}

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
        sw->doc->action(dc, kid); break;  // sw dangling pointer on return
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

void MainWindow::fileChanged(const QString &path)
{
    // TODO
    qDebug() << path;
}

void MainWindow::tabClose(int idx)
{
    if (nb->count() <= 1) return;
    Widget *sw = getTabByIndex(idx);
    if (sw->doc->closeDocument())
    {
        return;
    }
    auto wid = nb->widget(idx);
    nb->removeTab(idx);
    wid->deleteLater();
    myApp.fhistory->rememberOpenFiles();
}

void MainWindow::tabChange(int idx)
{
    if (idx != -1)
    {
        Widget *sw = getTabByIndex(idx);
        sw->status(QString());
        sw->doc->updateFileName();
    }
}

void MainWindow::appendSubMenu(
        QMenu *menu, int tag, const QString &contents,
        const QString &help, bool general, int status, QActionGroup *group)
{
    QString item = contents;
    QString key;
    int pos = contents.indexOf(QChar::fromLatin1('\t'));
    if (pos >= 0)
    {
        item = contents.mid(0, pos);
        key = contents.mid(pos + 1);
    }
    key = myApp.cfg->read(item, key).toString();
    QString newcontents = item;
    if (!key.isEmpty())
    {
        newcontents += "\t" + key;
        createShortcut(key, tag);
    }
    QAction *m = new QAction(newcontents, menu);
    if (!help.isEmpty())
    {
        m->setToolTip(help);
        menu->setToolTipsVisible(true);
    }
    m->setProperty("kid", tag);
    if (group) m->setActionGroup(group);
    if (status >= 0)
    {
        m->setCheckable(true);
        m->setChecked(status);
    }
    connect(m, SIGNAL(triggered(bool)), this, SLOT(actionActivated()));
    menu->addAction(m);
    if (general) myApp.cfg->menuShortcutMap.insert(item, key);
}

void MainWindow::updateStatus(const Selection &s)
{
    if (!sbl[1]) return;
    Cell *c = s.getCell();
    if (!(c && s.xs)) return;
    sbl[3]->setText(tr("Size %1").arg(-c->text.relsize));
    sbl[2]->setText(tr("Width %1").arg(s.g->colwidths[s.x]));
    sbl[1]->setText(tr("Edited %1").arg(c->text.lastedit.toString(Qt::SystemLocaleShortDate)));
}

void MainWindow::deIconize()
{
    if (!isHidden())
    {
        // TODO
        // RequestUserAttention();
        return;
    }
    show();
    trayIcon->hide();
}

void MainWindow::tabsReset()
{
    for (int i = 0; i < nb->count(); i++)
    {
        getTabByIndex(i)->doc->rootgrid->resetChildren();
    }
}

void MainWindow::cycleTabs(int offset)
{
    auto numtabs = nb->count();
    offset = ((offset >= 0) ? 1 : numtabs - 1);  // normalize to non-negative wrt modulo
    nb->setCurrentIndex((nb->currentIndex() + offset) % numtabs);
}

void MainWindow::fileChangeWatch(const QString &file)
{
    watcher->addPath(file);
}

Widget *MainWindow::getTabByFileName(const QString &fn)
{
    for (int i = 0; i < nb->count(); i++)
    {
        auto sa = qobject_cast<QScrollArea*>(nb->widget(i));
        Q_ASSERT(sa);
        auto win = qobject_cast<Widget*>(sa->widget());
        Q_ASSERT(win);
        if (win->doc->filename == fn)
        {
            nb->setCurrentIndex(i);
            return win;
        }
    }
    return nullptr;
}

Widget *MainWindow::getTabByIndex(int i) const
{
    Q_ASSERT(i >= 0 && i < nb->count());
    auto sa = qobject_cast<QScrollArea*>(nb->widget(i));
    Q_ASSERT(sa);
    auto win = qobject_cast<Widget*>(sa->widget());
    Q_ASSERT(win);
    return win;
}

Widget *MainWindow::getCurTab()
{
    int idx = nb->currentIndex();
    return idx >= 0? getTabByIndex(nb->currentIndex()): nullptr;
}

void MainWindow::setPageTitle(const QString &fn, const QString &mods, int page)
{
    int curidx = nb->currentIndex();
    if (page < 0) page = curidx;
    if (page < 0) return;
    if (page == curidx)
    {
        setWindowTitle(QStringLiteral("TreeSheets - ") + fn + mods);
    }
    const QString &nt = fn.isEmpty()?
                QStringLiteral("<unnamed>") :
                QFileInfo(fn).fileName();
    nb->setTabText(page, nt + mods);
}

void MainWindow::createShortcut(const QVariant &key, int kid)
{
    const QKeySequence &ks = key.type() != QVariant::String?
                QKeySequence(key.toInt()):
                QKeySequence(key.toString());
    auto k = ks.toString();
    if (k.isEmpty() || shortcut_bak.contains(k)) return;
    shortcut_bak.append(k);
    auto* shortcut = new QShortcut(ks, this, SLOT(actionActivated()));
    shortcut->setProperty("kid", kid);
}

void MainWindow::initUI()
{
    // 设置程序 icon
    QString imgspath = Tools::resolvePath(IMG_FILEPATH, true);
    if (!imgspath.isEmpty())
    {
        setWindowIcon(QIcon(imgspath + ICON_FILEPATH));
    }

    // 托盘功能
    trayIcon = new QSystemTrayIcon(this);
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->setIcon(QIcon(imgspath + ICON_FILEPATH));
    trayIcon->setToolTip(tr("TreeSheets"));

    {
        QString p = Tools::resolvePath(QStringLiteral("images/nuvola/fold.png"), true);
        if (!p.isEmpty()) foldicon = ImagePtr(new Image(QImage(p), 3));
        p =  Tools::resolvePath(QStringLiteral("images/render/line_nw.png"), true);
        if (!p.isEmpty()) line_nw = ImagePtr(new Image(QImage(p), 1));
        p =  Tools::resolvePath(QStringLiteral("images/render/line_sw.png"), true);
        if (!p.isEmpty()) line_sw = ImagePtr(new Image(QImage(p), 1));
    }

    bool mergetbar = myApp.cfg->read(QStringLiteral("mergetbar"), true).toBool();
    bool showtbar = myApp.cfg->read(QStringLiteral("showtbar"), true).toBool();
    bool showsbar = myApp.cfg->read(QStringLiteral("showsbar"), true).toBool();
    bool lefttabs = myApp.cfg->read(QStringLiteral("lefttabs"), true).toBool();
    bool iconset = myApp.cfg->read(QStringLiteral("iconset"), false).toBool();


    // 导出菜单
    QMenu*expmenu = new QMenu(tr("Export &view as"));
    {
        appendSubMenu(expmenu, A_EXPXML,
                      tr("&XML..."),
                      tr("Export the current view as XML (which can also be reimported without losing structure)"));
        appendSubMenu(expmenu, A_EXPHTMLT,
                      tr("&HTML (Tables+Styling)..."),
                      tr("Export the current view as HTML using nested tables, that will look somewhat like the "
                         "TreeSheet"));
        appendSubMenu(expmenu, A_EXPHTMLB,
                      tr("HTML (&Bullet points)..."),
                      tr("Export the current view as HTML as nested bullet points."));
        appendSubMenu(expmenu, A_EXPHTMLO,
                      tr("HTML (&Outline)..."),
                      tr("Export the current view as HTML as nested headers, suitable for importing into "
                         "Word's outline mode"));
        appendSubMenu(expmenu, A_EXPTEXT,
                      tr("Indented &Text..."),
                      tr("Export the current view as tree structured text, using spaces for each "
                         "indentation level. "
                         "Suitable for importing into mindmanagers and general text programs"));
        appendSubMenu(expmenu, A_EXPCSV,
                      tr("&Comma delimited text (CSV)..."),
                      tr("Export the current view as CSV. Good for spreadsheets and databases. Only works "
                         "on grids "
                         "with no sub-grids (use the Flatten operation first if need be)"));
        appendSubMenu(expmenu, A_EXPIMAGE,
                      tr("&Image..."),
                      tr("Export the current view as an image. Useful for faithfull renderings of the "
                         "TreeSheet, and "
                         "programs that don't accept any of the above options"));
    }
    // 导入菜单
    QMenu *impmenu = new QMenu(tr("Import file from"));
    {
        appendSubMenu(impmenu, A_IMPXML, tr("XML..."));
        appendSubMenu(impmenu, A_IMPXMLA, tr("XML (attributes too, for OPML etc)..."));
        appendSubMenu(impmenu, A_IMPTXTI, tr("Indented text..."));
        appendSubMenu(impmenu, A_IMPTXTC, tr("Comma delimited text (CSV)..."));
        appendSubMenu(impmenu, A_IMPTXTS, tr("Semi-Colon delimited text (CSV)..."));
        appendSubMenu(impmenu, A_IMPTXTT, tr("Tab delimited text..."));
    }
    // 历史记录菜单
    QMenu *recentmenu = new QMenu(tr("&Recent files"));
    myApp.fhistory->setMenu(recentmenu);
    // 文件
    QMenu *filemenu = new QMenu(tr("&File"), this);
    {
        appendSubMenu(filemenu, A_NEW, tr("&New\tCTRL+n"));
        appendSubMenu(filemenu, A_OPEN, tr("&Open...\tCTRL+o"));
        appendSubMenu(filemenu, A_CLOSE, tr("&Close\tCTRL+w"));
        filemenu->addMenu(recentmenu);
        appendSubMenu(filemenu, A_SAVE, tr("&Save\tCTRL+s"));
        appendSubMenu(filemenu, A_SAVEAS, tr("Save &As..."));
        filemenu->addSeparator();
        appendSubMenu(filemenu, A_PAGESETUP, tr("Page Setup..."));
        appendSubMenu(filemenu, A_PRINTSCALE, tr("Set Print Scale..."));
        appendSubMenu(filemenu, A_PREVIEW, tr("Print preview..."));
        appendSubMenu(filemenu, A_PRINT, tr("&Print...\tCTRL+p"));
        filemenu->addSeparator();
        filemenu->addMenu(expmenu);
        filemenu->addMenu(impmenu);
        filemenu->addSeparator();
        appendSubMenu(filemenu, A_EXIT, tr("&Exit\tCTRL+q"));
    }
    // 编辑菜单
    QMenu *editmenu;
    for(int twoeditmenus = 0; twoeditmenus < 2; twoeditmenus++)
    {
        QMenu *sizemenu = new QMenu(tr("Text Sizing..."));
        appendSubMenu(sizemenu, A_INCSIZE, tr("&Increase text size (SHIFT+mousewheel)\tSHIFT+PGUP"));
        appendSubMenu(sizemenu, A_DECSIZE, tr("&Decrease text size (SHIFT+mousewheel)\tSHIFT+PGDOWN"));
        appendSubMenu(sizemenu, A_RESETSIZE, tr("&Reset text sizes\tCTRL+SHIFT+s"));
        appendSubMenu(sizemenu, A_MINISIZE, tr("&Shrink text of all sub-grids\tCTRL+SHIFT+m"));
        sizemenu->addSeparator();
        appendSubMenu(sizemenu, A_INCWIDTH, tr("Increase column width (ALT+mousewheel)\tALT+PGUP"));
        appendSubMenu(sizemenu, A_DECWIDTH, tr("Decrease column width (ALT+mousewheel)\tALT+PGDOWN"));
        appendSubMenu(sizemenu, A_INCWIDTHNH,
                      tr("Increase column width (no sub grids)\tCTRL+ALT+PGUP"));
        appendSubMenu(sizemenu, A_DECWIDTHNH,
                      tr("Decrease column width (no sub grids)\tCTRL+ALT+PGDOWN"));
        appendSubMenu(sizemenu, A_RESETWIDTH, tr("Reset column widths\tCTRL+SHIFT+w"));

        QMenu *bordmenu = new QMenu(tr("Set Grid Border Width..."));
        appendSubMenu(bordmenu, A_BORD0, tr("Border &0\tCTRL+SHIFT+9"));
        appendSubMenu(bordmenu, A_BORD1, tr("Border &1\tCTRL+SHIFT+1"));
        appendSubMenu(bordmenu, A_BORD2, tr("Border &2\tCTRL+SHIFT+2"));
        appendSubMenu(bordmenu, A_BORD3, tr("Border &3\tCTRL+SHIFT+3"));
        appendSubMenu(bordmenu, A_BORD4, tr("Border &4\tCTRL+SHIFT+4"));
        appendSubMenu(bordmenu, A_BORD5, tr("Border &5\tCTRL+SHIFT+5"));

        QMenu *selmenu = new QMenu(tr("&Selection..."));
        appendSubMenu(selmenu, A_NEXT, tr("Move to next cell\tTAB"));
        appendSubMenu(selmenu, A_PREV, tr("Move to previous cell\tSHIFT+TAB"));
        selmenu->addSeparator();
        appendSubMenu(selmenu, A_SELALL, tr("Select &all in current grid\tCTRL+a"));
        selmenu->addSeparator();
        appendSubMenu(selmenu, A_LEFT, tr("Move Selection Left\tLEFT"));
        appendSubMenu(selmenu, A_RIGHT, tr("Move Selection Right\tRIGHT"));
        appendSubMenu(selmenu, A_UP, tr("Move Selection Up\tUP"));
        appendSubMenu(selmenu, A_DOWN, tr("Move Selection Down\tDOWN"));
        selmenu->addSeparator();
        appendSubMenu(selmenu, A_MLEFT, tr("Move Cells Left\tCTRL+LEFT"));
        appendSubMenu(selmenu, A_MRIGHT, tr("Move Cells Right\tCTRL+RIGHT"));
        appendSubMenu(selmenu, A_MUP, tr("Move Cells Up\tCTRL+UP"));
        appendSubMenu(selmenu, A_MDOWN, tr("Move Cells Down\tCTRL+DOWN"));
        selmenu->addSeparator();
        appendSubMenu(selmenu, A_SLEFT, tr("Extend Selection Left\tSHIFT+LEFT"));
        appendSubMenu(selmenu, A_SRIGHT, tr("Extend Selection Right\tSHIFT+RIGHT"));
        appendSubMenu(selmenu, A_SUP, tr("Extend Selection Up\tSHIFT+UP"));
        appendSubMenu(selmenu, A_SDOWN, tr("Extend Selection Down\tSHIFT+DOWN"));
        appendSubMenu(selmenu, A_SCOLS, tr("Extend Selection Full Columns"));
        appendSubMenu(selmenu, A_SROWS, tr("Extend Selection Full Rows"));
        selmenu->addSeparator();
        appendSubMenu(selmenu, A_CANCELEDIT, tr("Select &Parent\tESC"));
        appendSubMenu(selmenu, A_ENTERGRID, tr("Select First &Child\tSHIFT+ENTER"));
        selmenu->addSeparator();
        appendSubMenu(selmenu, A_LINK, tr("Go To &Matching Cell\tF6"));
        appendSubMenu(selmenu, A_LINKREV, tr("Go To Matching Cell (Reverse)\tSHIFT+F6"));

        QMenu *temenu = new QMenu(tr("Text &Editing..."));
        appendSubMenu(temenu, A_LEFT, tr("Cursor Left\tLEFT"));
        appendSubMenu(temenu, A_RIGHT, tr("Cursor Right\tRIGHT"));
        appendSubMenu(temenu, A_MLEFT, tr("Word Left\tCTRL+LEFT"));
        appendSubMenu(temenu, A_MRIGHT, tr("Word Right\tCTRL+RIGHT"));
        temenu->addSeparator();
        appendSubMenu(temenu, A_SLEFT, tr("Extend Selection Left\tSHIFT+LEFT"));
        appendSubMenu(temenu, A_SRIGHT, tr("Extend Selection Right\tSHIFT+RIGHT"));
        appendSubMenu(temenu, A_SCLEFT, tr("Extend Selection Word Left\tCTRL+SHIFT+LEFT"));
        appendSubMenu(temenu, A_SCRIGHT, tr("Extend Selection Word Right\tCTRL+SHIFT+RIGHT"));
        appendSubMenu(temenu, A_SHOME, tr("Extend Selection to Start\tSHIFT+HOME"));
        appendSubMenu(temenu, A_SEND, tr("Extend Selection to End\tSHIFT+END"));
        temenu->addSeparator();
        appendSubMenu(temenu, A_HOME, tr("Start of line of text\tHOME"));
        appendSubMenu(temenu, A_END, tr("End of line of text\tEND"));
        appendSubMenu(temenu, A_CHOME, tr("Start of text\tCTRL+HOME"));
        appendSubMenu(temenu, A_CEND, tr("End of text\tCTRL+END"));
        temenu->addSeparator();
        appendSubMenu(temenu, A_ENTERCELL, tr("Enter/exit text edit mode\tENTER"));
        appendSubMenu(temenu, A_ENTERCELL, tr("Enter/exit text edit mode\tF2"));
        appendSubMenu(temenu, A_PROGRESSCELL, tr("Enter/exit text edit to the right\tALT+ENTER"));
        appendSubMenu(temenu, A_CANCELEDIT, tr("Cancel text edits\tESC"));

        QMenu *stmenu = new QMenu(tr("Text Style..."));
        appendSubMenu(stmenu, A_BOLD, tr("Toggle cell &BOLD\tCTRL+b"));
        appendSubMenu(stmenu, A_ITALIC, tr("Toggle cell &ITALIC\tCTRL+i"));
        appendSubMenu(stmenu, A_TT, tr("Toggle cell &typewriter\tCTRL+ALT+t"));
        appendSubMenu(stmenu, A_UNDERL, tr("Toggle cell &underlined\tCTRL+u"));
        appendSubMenu(stmenu, A_STRIKET, tr("Toggle cell &strikethrough\tCTRL+t"));
        stmenu->addSeparator();
        appendSubMenu(stmenu, A_RESETSTYLE, tr("&Reset text styles\tCTRL+SHIFT+r"));
        appendSubMenu(stmenu, A_RESETCOLOR, tr("Reset &colors\tCTRL+SHIFT+c"));
        stmenu->addSeparator();
        appendSubMenu(stmenu, A_LASTCELLCOLOR, tr("Apply last cell color\tSHIFT+ALT+c"));
        appendSubMenu(stmenu, A_LASTTEXTCOLOR, tr("Apply last text color\tSHIFT+ALT+t"));
        appendSubMenu(stmenu, A_LASTBORDCOLOR, tr("Apply last border color\tSHIFT+ALT+b"));
        appendSubMenu(stmenu, A_OPENCELLCOLOR, tr("Open cell colors\tSHIFT+ALT+F9"));
        appendSubMenu(stmenu, A_OPENTEXTCOLOR, tr("Open text colors\tSHIFT+ALT+F10"));
        appendSubMenu(stmenu, A_OPENBORDCOLOR, tr("Open border colors\tSHIFT+ALT+F11"));

        QMenu *tagmenu = new QMenu(tr("Tag..."));
        appendSubMenu(tagmenu, A_TAGADD, tr("&Add Cell Text as Tag"));
        appendSubMenu(tagmenu, A_TAGREMOVE, tr("&Remove Cell Text from Tags"));
        appendSubMenu(tagmenu, A_NOP, tr("&Set Cell Text to tag (use CTRL+RMB)"),
                      tr("Hold CTRL while pressing right mouse button to quickly set a tag for the "
                         "current cell "
                         "using a popup menu"));

        QMenu *orgmenu = new QMenu(tr("&Grid Reorganization..."));
        appendSubMenu(orgmenu, A_TRANSPOSE, tr("&Transpose\tCTRL+SHIFT+t"),
                      tr("changes the orientation of a grid"));
        appendSubMenu(orgmenu, A_SORT, tr("Sort &Ascending"),
                      tr("Make a 1xN selection to indicate which column to sort on, and which rows to "
                         "affect"));
        appendSubMenu(orgmenu, A_SORTD, tr("Sort &Descending"),
                      tr("Make a 1xN selection to indicate which column to sort on, and which rows to "
                         "affect"));
        appendSubMenu(orgmenu, A_HSWAP, tr("Hierarchy &Swap\tF8"),
                      tr("Swap all cells with this text at this level (or above) with the parent"));
        appendSubMenu(orgmenu, A_HIFY, tr("&Hierarchify"),
                      tr("Convert an NxN grid with repeating elements per column into an 1xN grid "
                         "with hierarchy, "
                         "useful to convert data from spreadsheets"));
        appendSubMenu(orgmenu, A_FLATTEN, tr("&Flatten"),
                      tr("Takes a hierarchy (nested 1xN or Nx1 grids) and converts it into a flat NxN "
                         "grid, useful "
                         "for export to spreadsheets"));

        QMenu *imgmenu = new QMenu(tr("&Images..."));
        appendSubMenu(imgmenu, A_IMAGE, tr("&Add Image"), tr("Adds an image to the selected cell"));
        appendSubMenu(imgmenu, A_IMAGESCP, tr("&Scale Image (re-sample pixels)"),
                      tr("Change the image size if it is too big, by reducing the amount of "
                         "pixels"));
        appendSubMenu(imgmenu, A_IMAGESCF, tr("&Scale Image (display only)"),
                      tr("Change the image size if it is too big or too small, by changing the size "
                         "shown on screen. Applies to all uses of this image."));
        appendSubMenu(imgmenu, A_IMAGESCN, tr("&Reset Scale (display only)"),
                      tr("Change the scale to match DPI of the current display. "
                         "Applies to all uses of this image."));
        appendSubMenu(imgmenu, A_IMAGER, tr("&Remove Image(s)"),
                      tr("Remove image(s) from the selected cells"));

        QMenu *navmenu = new QMenu(tr("&Browsing..."));
        appendSubMenu(navmenu, A_BROWSE, tr("Open link in &browser\tF5"),
                      tr("Opens up the text from the selected cell in browser (should start be a "
                         "valid URL)"));
        appendSubMenu(navmenu, A_BROWSEF, tr("Open &file\tF4"),
                      tr("Opens up the text from the selected cell in default application for the "
                         "file type"));

        QMenu *laymenu = new QMenu(tr("&Layout && Render Style..."));
        appendSubMenu(laymenu, A_V_GS, tr("Vertical Layout with Grid Style Rendering\tALT+1"));
        appendSubMenu(laymenu, A_V_BS, tr("Vertical Layout with Bubble Style Rendering\tALT+2"));
        appendSubMenu(laymenu, A_V_LS, tr("Vertical Layout with Line Style Rendering\tALT+3"));
        laymenu->addSeparator();
        appendSubMenu(laymenu, A_H_GS, tr("Horizontal Layout with Grid Style Rendering\tALT+4"));
        appendSubMenu(laymenu, A_H_BS, tr("Horizontal Layout with Bubble Style Rendering\tALT+5"));
        appendSubMenu(laymenu, A_H_LS, tr("Horizontal Layout with Line Style Rendering\tALT+6"));
        laymenu->addSeparator();
        appendSubMenu(laymenu, A_GS, tr("Grid Style Rendering\tALT+7"));
        appendSubMenu(laymenu, A_BS, tr("Bubble Style Rendering\tALT+8"));
        appendSubMenu(laymenu, A_LS, tr("Line Style Rendering\tALT+9"));
        laymenu->addSeparator();
        appendSubMenu(laymenu, A_TEXTGRID, tr("Toggle Vertical Layout\tF7"),
                      tr("Make a hierarchy layout more vertical (default) or more horizontal"));

        editmenu = new QMenu(tr("&Edit"), this);
        appendSubMenu(editmenu, A_CUT, tr("Cu&t\tCTRL+x"));
        appendSubMenu(editmenu, A_COPY, tr("&Copy\tCTRL+c"));
        appendSubMenu(editmenu, A_COPYCT, tr("Copy As Continuous Text"));
        appendSubMenu(editmenu, A_PASTE, tr("&Paste\tCTRL+v"));
        appendSubMenu(editmenu, A_PASTESTYLE, tr("Paste Style Only\tCTRL+SHIFT+v"),
                      tr("only sets the colors and style of the copied cell, and keeps the text"));
        editmenu->addSeparator();
        appendSubMenu(editmenu, A_UNDO, tr("&Undo\tCTRL+z"), tr("revert the changes, one step at a time"));
        appendSubMenu(editmenu, A_REDO, tr("&Redo\tCTRL+y"),
                      tr("redo any undo steps, if you haven't made changes since"));
        editmenu->addSeparator();
        appendSubMenu(editmenu, A_DELETE, tr("&Delete After\tDEL"),
                      tr("Deletes the column of cells after the selected grid line, or the row below"));
        appendSubMenu(
                    editmenu, A_BACKSPACE, tr("Delete Before\tBACK"),
                    tr("Deletes the column of cells before the selected grid line, or the row above"));
        appendSubMenu(editmenu, A_DELETE_WORD, tr("Delete Word After\tCTRL+DEL"),
                      tr("Deletes the entire word after the cursor"));
        appendSubMenu(editmenu, A_BACKSPACE_WORD, tr("Delete Word Before\tCTRL+BACK"),
                      tr("Deletes the entire word before the cursor"));
        editmenu->addSeparator();
        appendSubMenu(editmenu, A_NEWGRID,
              #ifdef Q_OS_MAC
                      tr("&Insert New Grid\tCTRL+g"),
              #else
                      tr("&Insert New Grid\tINS"),
              #endif
                      tr("Adds a grid to the selected cell"));
        appendSubMenu(editmenu, A_WRAP, tr("&Wrap in new parent\tF9"),
                      tr("Creates a new level of hierarchy around the current selection"));
        editmenu->addSeparator();
        // F10 is tied to the OS on both Ubuntu and OS X, and SHIFT+F10 is now right
        // click on all platforms?
        appendSubMenu(editmenu, A_FOLD,
              #ifndef Q_OS_WIN
                      tr("Toggle Fold\tCTRL+F10"),
              #else
                      tr("Toggle Fold\tF10"),
              #endif
                      tr("Toggles showing the grid of the selected cell(s)"));
        appendSubMenu(editmenu, A_FOLDALL, tr("Fold All\tCTRL+SHIFT+F10"),
                      tr("Folds the grid of the selected cell(s) recursively"));
        appendSubMenu(editmenu, A_UNFOLDALL, tr("Unfold All\tCTRL+ALT+F10"),
                      tr("Unfolds the grid of the selected cell(s) recursively"));
        editmenu->addSeparator();
        editmenu->addMenu(selmenu);
        editmenu->addMenu(orgmenu);
        editmenu->addMenu(laymenu);
        editmenu->addMenu(imgmenu);
        editmenu->addMenu(navmenu);
        editmenu->addMenu(temenu);
        editmenu->addMenu(sizemenu);
        editmenu->addMenu(stmenu);
        editmenu->addMenu(bordmenu);
        editmenu->addMenu(tagmenu);

        if (!twoeditmenus) editmenupopup = editmenu;
    }
    // 搜索
    QMenu *semenu = new QMenu(tr("&Search"), this);
    {
        appendSubMenu(semenu, A_SEARCHF, tr("&Search\tCTRL+f"));
        appendSubMenu(semenu, A_SEARCHNEXT, tr("&Go To Next Search Result\tF3"));
        appendSubMenu(semenu, A_REPLACEONCE, tr("&Replace in Current Selection\tCTRL+h"));
        appendSubMenu(semenu, A_REPLACEONCEJ, tr("&Replace in Current Selection & Jump Next\tCTRL+j"));
        appendSubMenu(semenu, A_REPLACEALL, tr("Replace &All"));
    }
    //
    QMenu *scrollmenu = new QMenu(tr("Scroll Sheet..."));
    {
        appendSubMenu(scrollmenu, A_AUP, tr("Scroll Up (mousewheel)\tPGUP"));
        appendSubMenu(scrollmenu, A_AUP, tr("Scroll Up (mousewheel)\tALT+UP"));
        appendSubMenu(scrollmenu, A_ADOWN, tr("Scroll Down (mousewheel)\tPGDOWN"));
        appendSubMenu(scrollmenu, A_ADOWN, tr("Scroll Down (mousewheel)\tALT+DOWN"));
        appendSubMenu(scrollmenu, A_ALEFT, tr("Scroll Left\tALT+LEFT"));
        appendSubMenu(scrollmenu, A_ARIGHT, tr("Scroll Right\tALT+RIGHT"));
    }
    //
    QMenu *filtermenu = new QMenu(tr("Filter..."));
    {
        appendSubMenu(filtermenu, A_FILTEROFF, tr("Turn filter &off"));
        appendSubMenu(filtermenu, A_FILTERS, tr("Show only cells in current search"));
        appendSubMenu(filtermenu, A_FILTER5, tr("Show 5% of last edits"));
        appendSubMenu(filtermenu, A_FILTER10, tr("Show 10% of last edits"));
        appendSubMenu(filtermenu, A_FILTER20, tr("Show 20% of last edits"));
        appendSubMenu(filtermenu, A_FILTER50, tr("Show 50% of last edits"));
        appendSubMenu(filtermenu, A_FILTERM, tr("Show 1% more than the last filter"));
        appendSubMenu(filtermenu, A_FILTERL, tr("Show 1% less than the last filter"));
    }
    //
    QMenu *viewmenu = new QMenu(tr("&View"), this);
    {
        appendSubMenu(viewmenu, A_ZOOMIN, tr("Zoom &In (CTRL+mousewheel)\tCTRL+PGUP"));
        appendSubMenu(viewmenu, A_ZOOMOUT, tr("Zoom &Out (CTRL+mousewheel)\tCTRL+PGDOWN"));
        appendSubMenu(viewmenu, A_NEXTFILE, tr("Switch to &next file/tab\tCTRL+TAB"));
        appendSubMenu(viewmenu, A_PREVFILE, tr("Switch to &previous file/tab\tCTRL+SHIFT+TAB"));
        appendSubMenu(viewmenu, A_FULLSCREEN,
#ifdef Q_OS_MAC
                      tr("Toggle &Fullscreen View\tCTRL+F11"));
#else
                      tr("Toggle &Fullscreen View\tF11"));
#endif
        appendSubMenu(viewmenu, A_SCALED,
#ifdef Q_OS_MAC
                      tr("Toggle &Scaled Presentation View\tCTRL+F12"));
#else
                      tr("Toggle &Scaled Presentation View\tF12"));
#endif
        appendSubMenu(viewmenu, A_ZEN_MODE, tr("Toggle Zen Mode"));
        viewmenu->addMenu(scrollmenu);
        viewmenu->addMenu(filtermenu);
    }
    //
    QMenu *roundmenu = new QMenu(tr("&Roundness of grid borders..."));
    {
        QActionGroup *roundgroup = new QActionGroup(roundmenu);
        int roundcheck = myApp.cfg->roundness;
        appendSubMenu(roundmenu, A_ROUND0, tr("Radius &0"), QString(), false, roundcheck == 0, roundgroup);
        appendSubMenu(roundmenu, A_ROUND1, tr("Radius &1"), QString(), false, roundcheck == 1, roundgroup);
        appendSubMenu(roundmenu, A_ROUND2, tr("Radius &2"), QString(), false, roundcheck == 2, roundgroup);
        appendSubMenu(roundmenu, A_ROUND3, tr("Radius &3"), QString(), false, roundcheck == 3, roundgroup);
        appendSubMenu(roundmenu, A_ROUND4, tr("Radius &4"), QString(), false, roundcheck == 4, roundgroup);
        appendSubMenu(roundmenu, A_ROUND5, tr("Radius &5"), QString(), false, roundcheck == 5, roundgroup);
        appendSubMenu(roundmenu, A_ROUND6, tr("Radius &6"), QString(), false, roundcheck == 6, roundgroup);
    }
    //
    QMenu *optmenu = new QMenu(tr("&Options"), this);
    {
        appendSubMenu(optmenu, A_DEFFONT, tr("Pick Default Font..."));
        appendSubMenu(optmenu, A_CUSTKEY, tr("Change a key binding..."));
        appendSubMenu(optmenu, A_CUSTCOL, tr("Pick Custom &Color..."));
        appendSubMenu(optmenu, A_COLCELL, tr("&Set Custom Color From Cell BG"));
        appendSubMenu(optmenu, A_DEFBGCOL, tr("Pick Document Background..."));
        optmenu->addSeparator();
        appendSubMenu(optmenu, A_SHOWSBAR, tr("Show Statusbar"), QString(), false, showsbar);
        appendSubMenu(optmenu, A_SHOWTBAR, tr("Show Toolbar"), QString(), false, showtbar);
        appendSubMenu(optmenu, A_LEFTTABS, tr("File Tabs on the bottom"), QString(), false, lefttabs);
        appendSubMenu(optmenu, A_TOTRAY, tr("Minimize to tray"), QString(), false, myApp.cfg->totray);
        appendSubMenu(optmenu, A_MINCLOSE, tr("Minimize on close"), QString(), false, myApp.cfg->minclose);
        appendSubMenu(optmenu, A_SINGLETRAY, tr("Single click maximize from tray"), QString(), false, myApp.cfg->singletray);
        optmenu->addSeparator();
        appendSubMenu(optmenu, A_ZOOMSCR, tr("Swap mousewheel scrolling and zooming"), QString(), false, myApp.cfg->zoomscroll);
        appendSubMenu(optmenu, A_THINSELC, tr("Navigate in between cells with cursor keys"), QString(), false, myApp.cfg->thinselc);
        optmenu->addSeparator();
        appendSubMenu(optmenu, A_MAKEBAKS, tr("Create .bak files"), QString(), false, myApp.cfg->makebaks);
        appendSubMenu(optmenu, A_AUTOSAVE, tr("Autosave to .tmp"), QString(), false, myApp.cfg->autosave);
        appendSubMenu(optmenu, A_FSWATCH, tr("Auto reload documents"), tr("Reloads when another computer has changed a file (if you have made changes, asks)"), false, myApp.cfg->fswatch);
        appendSubMenu(optmenu, A_AUTOEXPORT,  tr("Automatically export a .html on every save"), QString(), false, myApp.cfg->autohtmlexport);
        optmenu->addSeparator();
        appendSubMenu(optmenu, A_CENTERED,  tr("Render document centered"), QString(), false, myApp.cfg->centered);
        appendSubMenu(optmenu, A_FASTRENDER,  tr("Faster line rendering"), QString(), false, myApp.cfg->fastrender);
        appendSubMenu(optmenu, A_ICONSET,  tr("Black and white toolbar icons"), QString(), false, iconset);
        optmenu->addMenu(roundmenu);
    }
    //
    QMenu *scriptmenu = new QMenu(tr("Script"), this);
    {
        auto scriptpath = Tools::resolvePath("scripts", true);
        if (!scriptpath.isEmpty())
        {
            int sidx = 0;
            const QStringList &ls = QDir(scriptpath).entryList(QStringList() << QStringLiteral("*.lobster"), QDir::Files);
            foreach (const QString &fn, ls)
            {
                auto ms = fn.section(QChar::fromLatin1('.'), 0, -2);
                if (sidx < 26) {
                    ms += QStringLiteral("\tCTRL+SHIFT+ALT+");
                    ms += QChar::fromLatin1('A' + sidx);
                }
                appendSubMenu(scriptmenu, A_SCRIPT + sidx, ms);
                myApp.cfg->scriptsInMenu << fn;
                sidx++;
            }
        }
    }
    //
    QMenu *markmenu = new QMenu(tr("&Mark as"));
    {
        appendSubMenu(markmenu, A_MARKDATA, tr("&Data\tCTRL+ALT+d"));
        appendSubMenu(markmenu, A_MARKCODE, tr("&Operation\tCTRL+ALT+o"));
        appendSubMenu(markmenu, A_MARKVARD, tr("Variable &Assign\tCTRL+ALT+a"));
        appendSubMenu(markmenu, A_MARKVARU, tr("Variable &Read\tCTRL+ALT+r"));
        appendSubMenu(markmenu, A_MARKVIEWH, tr("&Horizontal View\tCTRL+ALT+."));
        appendSubMenu(markmenu, A_MARKVIEWV, tr("&Vertical View\tCTRL+ALT+,"));
    }
    //
    QMenu *langmenu = new QMenu(tr("&Program"), this);
    {
        appendSubMenu(langmenu, A_RUN, tr("&Run\tCTRL+ALT+F5"));
        langmenu->addMenu(markmenu);
        appendSubMenu(langmenu, A_CLRVIEW, tr("&Clear Views"));
    }
    //
    QMenu *helpmenu = new QMenu(tr("&Help"), this);
    {
        appendSubMenu(helpmenu, A_ABOUT, tr("&About..."));
        appendSubMenu(helpmenu, A_ABOUTQT, tr("&About Qt..."));
        appendSubMenu(helpmenu, A_HELPI, tr("Load interactive &tutorial...\tF1"));
        appendSubMenu(helpmenu, A_HELP_OP_REF, tr("Load operation reference...\tCTRL+ALT+F1"));
        appendSubMenu(helpmenu, A_HELP, tr("View tutorial &web page..."));
    }

    createShortcut(Qt::SHIFT + Qt::Key_Delete, A_CUT);
    createShortcut(Qt::SHIFT + Qt::Key_Insert, A_PASTE);
    createShortcut(Qt::CTRL + Qt::Key_Insert, A_COPY);

    if (mergetbar)
    {
        QMenuBar *menubar = new QMenuBar(this);
        menubar->addMenu(filemenu);
        menubar->addMenu(editmenu);
        menubar->addMenu(semenu);
        menubar->addMenu(viewmenu);
        menubar->addMenu(optmenu);
        menubar->addMenu(scriptmenu);
        menubar->addMenu(langmenu);
        menubar->addMenu(helpmenu);
        setMenuBar(menubar);
    }
    else { } // 没有用到的上述菜单指针，被全局管理

    const QString &toolbgcol = iconset ? QStringLiteral(TOOL_BGCOLOR0) : QStringLiteral(TOOL_BGCOLOR1);
    if (showtbar || mergetbar)
    {
        tb = new QToolBar(this);
        tb->setMovable(false);

        tb->setStyleSheet(QStringLiteral("QToolBar{background: %1;}").arg(toolbgcol));

        const QString iconpath =
            Tools::resolvePath(iconset ? TOOL_ICON0: TOOL_ICON1, false);
        auto sz = iconset ? QSize(TOOL_SIZE0, TOOL_SIZE0) : QSize(TOOL_SIZE1, TOOL_SIZE1);
        tb->setIconSize(sz);

        //double sc = iconset ? 1.0 : 22.0 / 48.0;
#ifdef Q_OS_MAC
#define SEPARATOR
#else
#define SEPARATOR tb->addSeparator()
#endif
        auto AddTBIcon = [&](const QString &name, int action, const QString &file) {
            QAction*m = tb->addAction(QIcon(file), name, this, SLOT(actionActivated()));
            m->setToolTip(name);
            m->setProperty("kid", action);
        };

        AddTBIcon(tr("New (CTRL+n)"), A_NEW, iconpath + QStringLiteral("/filenew.png"));
        AddTBIcon(tr("Open (CTRL+o)"), A_OPEN, iconpath + QStringLiteral("/fileopen.png"));
        AddTBIcon(tr("Save (CTRL+s)"), A_SAVE, iconpath + QStringLiteral("/filesave.png"));
        AddTBIcon(tr("Save As"), A_SAVEAS, iconpath + QStringLiteral("/filesaveas.png"));
        SEPARATOR;
        AddTBIcon(tr("Undo (CTRL+z)"), A_UNDO, iconpath + QStringLiteral("/undo.png"));
        AddTBIcon(tr("Copy (CTRL+c)"), A_COPY, iconpath + QStringLiteral("/editcopy.png"));
        AddTBIcon(tr("Paste (CTRL+v)"), A_PASTE, iconpath + QStringLiteral("/editpaste.png"));
        SEPARATOR;
        AddTBIcon(tr("Zoom In (CTRL+mousewheel)"), A_ZOOMIN, iconpath + QStringLiteral("/zoomin.png"));
        AddTBIcon(tr("Zoom Out (CTRL+mousewheel)"), A_ZOOMOUT, iconpath + QStringLiteral("/zoomout.png"));
        SEPARATOR;
        AddTBIcon(tr("New Grid (INS)"), A_NEWGRID, iconpath + QStringLiteral("/newgrid.png"));
        AddTBIcon(tr("Add Image"), A_IMAGE, iconpath + QStringLiteral("/image.png"));
        SEPARATOR;
        AddTBIcon(tr("Run"), A_RUN, iconpath + QStringLiteral("/run.png"));
        SEPARATOR;

        tb->addWidget(new QLabel(tr("Search ")));
        tb->addWidget(filter = new QLineEdit);
        filter->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        filter->setMinimumWidth(80);
        filter->setMaximumWidth(80);
        filter->setProperty("kid", A_SEARCH);
        connect(filter, SIGNAL(returnPressed()), this, SLOT(actionActivated()));
        SEPARATOR;

        tb->addWidget(new QLabel(tr("Replace ")));
        tb->addWidget(replaces = new QLineEdit);
        replaces->setMinimumWidth(60);
        replaces->setMaximumWidth(60);
        replaces->setProperty("kid", A_REPLACE);
        connect(replaces, SIGNAL(returnPressed()), this, SLOT(actionActivated()));
        SEPARATOR;

        tb->addWidget(new QLabel(tr("Cell ")));
        tb->addWidget(celldd = new ColorDropdown(1, 70, 20, tb));
        celldd->setProperty("kid", A_CELLCOLOR);
        connect(celldd, SIGNAL(currentIndexChanged(int)), this, SLOT(actionActivated()));
        SEPARATOR;

        tb->addWidget(new QLabel(tr("Text ")));
        tb->addWidget(textdd = new ColorDropdown(2, 70, 20, tb));
        textdd->setProperty("kid", A_TEXTCOLOR);
        connect(textdd, SIGNAL(currentIndexChanged(int)), this, SLOT(actionActivated()));
        SEPARATOR;

        tb->addWidget(new QLabel(tr("Border ")));
        tb->addWidget(borddd = new ColorDropdown(7, 70, 20, tb));
        borddd->setProperty("kid", A_BORDCOLOR);
        connect(borddd, SIGNAL(currentIndexChanged(int)), this, SLOT(actionActivated()));
        tb->addSeparator();

        tb->addWidget(new QLabel(tr("Image ")));
        tb->addWidget(idd = new ImageDropdown(0, 50, 20, Tools::resolvePath("images/nuvola/dropdown", false), tb));
        idd->setProperty("kid", A_DDIMAGE);
        connect(idd, SIGNAL(currentIndexChanged(int)), this, SLOT(actionActivated()));

        addToolBar(Qt::TopToolBarArea, tb);
    }
    else tb = nullptr;

    if (showsbar) {
        sb = new QStatusBar(this);
        sb->setStyleSheet(QStringLiteral("QStatusBar{background: %1;}").arg(toolbgcol));

        sb->addWidget(sbl[0] = new QLabel, 1);
        sb->addWidget(sbl[1] = new QLabel, 0);
        sb->addWidget(sbl[2] = new QLabel, 0);
        sb->addWidget(sbl[3] = new QLabel, 0);
        sbl[1]->setMinimumWidth(200);
        sbl[1]->setMaximumWidth(200);
        sbl[2]->setMinimumWidth(120);
        sbl[2]->setMaximumWidth(120);
        sbl[3]->setMinimumWidth(100);
        sbl[3]->setMaximumWidth(100);
        setStatusBar(sb);
    }
    else
    {
        sb = nullptr;
        sbl[0] = nullptr;
        sbl[1] = nullptr;
        sbl[2] = nullptr;
        sbl[3] = nullptr;
    }
    nb = new QTabWidget(this);
    nb->setMovable(true);
    nb->setTabsClosable(true);
    setCentralWidget(nb);
    nb->setTabPosition(QTabWidget::South);
    connect(nb, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClose(int)));
    connect(nb, SIGNAL(currentChanged(int)), this, SLOT(tabChange(int)));

    auto desktop = QApplication::desktop();
    QRect clientRect = desktop->availableGeometry(desktop->screenNumber(this));
    const int boundary = 64;
    const int screenx = clientRect.width() - clientRect.x();
    const int screeny = clientRect.height() - clientRect.y();
    const int defx = screenx - 2 * boundary;
    const int defy = screeny - 2 * boundary;

    int resx = myApp.cfg->read(QStringLiteral("resx"), defx).toInt();
    int resy = myApp.cfg->read(QStringLiteral("resy"), defy).toInt();
    int posx = myApp.cfg->read(QStringLiteral("posx"), boundary + clientRect.x()).toInt();
    int posy = myApp.cfg->read(QStringLiteral("posy"), boundary + clientRect.y()).toInt();

    const bool b =
            resx > screenx || resy > screeny ||
            posx < clientRect.x() || posy < clientRect.y() ||
            posx + resx > clientRect.width() + clientRect.x() ||
            posy + resy > clientRect.height() + clientRect.y();
    if (b)
    {
        resx = defx;
        resy = defy;
        posx = posy = boundary;
        posx += clientRect.x();
        posy += clientRect.y();
    }
    setGeometry(QRect(posx, posy, resx, resy));

    if (myApp.cfg->read(QStringLiteral("maximized"), true).toBool())
    {
        showMaximized();
    }
    else
    {
        show();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    bool fcb = fromclosebox;
    fromclosebox = true;
    if (fcb && myApp.cfg->minclose)
    {
        event->ignore();
        trayIcon->show();
        hide();
        return;
    }
    while (nb->count())
    {
        if (getCurTab()->doc->closeDocument())
        {
            event->ignore();
            // may have closed some, but not all
            myApp.fhistory->rememberOpenFiles();
            return;
        }
        else
        {
            auto wid = nb->currentWidget();
            nb->removeTab(nb->currentIndex());
            wid->deleteLater();
        }
    }

    myApp.fhistory->rememberOpenFiles();
    if (!isHidden())
    {
        bool isM = isMaximized();
        myApp.cfg->write(QStringLiteral("maximized"), isM);
        if (!isM)
        {
            const auto &rect = geometry();
            myApp.cfg->write(QStringLiteral("resx"), rect.width());
            myApp.cfg->write(QStringLiteral("resy"), rect.height());
            myApp.cfg->write(QStringLiteral("posx"), rect.x());
            myApp.cfg->write(QStringLiteral("posy"), rect.y());
        }
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    int tid = event->timerId();
    if (tid == blinkTimer)
    {
        int idx = nb->currentIndex();
        if (idx == -1) return;
        Widget*widget = getTabByIndex(idx);
        Q_ASSERT(widget);
        widget->doc->Blink();
    }
    else if (tid == savechecker)
    {
        for (int i = 0; i < nb->count(); i++)
        {
            Widget *widget = getTabByIndex(i);
            Q_ASSERT(widget);
            widget->doc->autoSave(!myApp.frame->isActiveWindow(), i);
        }

    }
}
