#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widget.h"
#include "tools.h"
#include "symdef.h"
#include "myapp.h"
#include "config.h"

#include <QScrollArea>
#include <QIcon>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUI();
}

MainWindow::~MainWindow()
{
    delete ui;
}

Widget *MainWindow::createWidget(const QString &name, bool append)
{
    QScrollArea*scroll = new QScrollArea;
    Q_ASSERT(scroll);
    Widget* widget = new Widget(scroll);
    Q_ASSERT(widget);
    ui->centralwidget->insertTab(append? -1: 0, scroll, name);
    return widget;
}

// 托盘图标事件
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    bool b = reason == (myApp.cfg->singletray?
                            QSystemTrayIcon::Trigger:
                            QSystemTrayIcon::DoubleClick);
    if (!b) return;

    qDebug() << reason;
}

static void appendSubMenu(QMenu *menu, int tag, const QString &contents, const QString &help = QString())
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
    if (!key.isEmpty()) newcontents += "\t" + key;

    QAction *m = new QAction(newcontents, menu);
    if (!help.isEmpty())
    {
        m->setToolTip(help);
        menu->setToolTipsVisible(true);
    }
    m->setData(tag);

    menu->addAction(m);
   // menustrings.push_back(std::make_pair(item, key));
}

void MainWindow::initUI()
{
    // 设置程序 icon
    QString imgspath = Tools::resolvePath(IMG_FILEPATH, true);
    if (!imgspath.isEmpty())
    {
        setWindowIcon(QIcon(imgspath + "/icon16.png"));
    }

    // 托盘功能
    trayIcon = new QSystemTrayIcon(this);
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->setIcon(QIcon(imgspath + "/icon16.png"));

    bool mergetbar = myApp.cfg->read(QStringLiteral("showtbar"), false).toBool();
    bool showtbar = myApp.cfg->read(QStringLiteral("showtbar"), true).toBool();
    bool showsbar = myApp.cfg->read(QStringLiteral("showtbar"), true).toBool();
    bool lefttabs = myApp.cfg->read(QStringLiteral("showtbar"), true).toBool();
    bool iconset = myApp.cfg->read(QStringLiteral("showtbar"), false).toBool();

    // 导出菜单
    QMenu*expmenu = new QMenu(tr("Export &view as"));
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
    // 导入菜单
    QMenu *impmenu = new QMenu(tr("Import file from"));
    appendSubMenu(impmenu, A_IMPXML, tr("XML..."));
    appendSubMenu(impmenu, A_IMPXMLA, tr("XML (attributes too, for OPML etc)..."));
    appendSubMenu(impmenu, A_IMPTXTI, tr("Indented text..."));
    appendSubMenu(impmenu, A_IMPTXTC, tr("Comma delimited text (CSV)..."));
    appendSubMenu(impmenu, A_IMPTXTS, tr("Semi-Colon delimited text (CSV)..."));
    appendSubMenu(impmenu, A_IMPTXTT, tr("Tab delimited text..."));
    // 历史记录菜单
    QMenu *recentmenu = new QMenu(tr("&Recent files"));
    // TODO

    // 文件
    QMenu *filemenu = new QMenu(tr("&File"));
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
    ui->menubar->addMenu(filemenu);


    // 编辑菜单
    for(int twoeditmenus = 0; twoeditmenus < 2; twoeditmenus++)
    {
        QMenu *sizemenu = new QMenu(tr("Text Sizing..."));
        appendSubMenu(sizemenu, A_INCSIZE, tr("&Increase text size (SHIFT+mousewheel)\tSHIFT+PGUP"));
        appendSubMenu(sizemenu, A_DECSIZE, tr("&Decrease text size (SHIFT+mousewheel)\tSHIFT+PGDN"));
        appendSubMenu(sizemenu, A_RESETSIZE, tr("&Reset text sizes\tCTRL+SHIFT+s"));
        appendSubMenu(sizemenu, A_MINISIZE, tr("&Shrink text of all sub-grids\tCTRL+SHIFT+m"));
        sizemenu->addSeparator();
        appendSubMenu(sizemenu, A_INCWIDTH, tr("Increase column width (ALT+mousewheel)\tALT+PGUP"));
        appendSubMenu(sizemenu, A_DECWIDTH, tr("Decrease column width (ALT+mousewheel)\tALT+PGDN"));
        appendSubMenu(sizemenu, A_INCWIDTHNH,
                 tr("Increase column width (no sub grids)\tCTRL+ALT+PGUP"));
        appendSubMenu(sizemenu, A_DECWIDTHNH,
                 tr("Decrease column width (no sub grids)\tCTRL+ALT+PGDN"));
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

        QMenu *editmenu = new QMenu(tr("&Edit"));
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
                 #ifndef WIN32
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

        if (twoeditmenus) ui->menubar->addMenu(editmenu);
        else editmenupopup = editmenu;
    }


    QMenu *semenu = new QMenu(tr("&Search"));
    appendSubMenu(semenu, A_SEARCHF, tr("&Search\tCTRL+f"));
    appendSubMenu(semenu, A_SEARCHNEXT, tr("&Go To Next Search Result\tF3"));
    appendSubMenu(semenu, A_REPLACEONCE, tr("&Replace in Current Selection\tCTRL+h"));
    appendSubMenu(semenu, A_REPLACEONCEJ, tr("&Replace in Current Selection & Jump Next\tCTRL+j"));
    appendSubMenu(semenu, A_REPLACEALL, tr("Replace &All"));
    ui->menubar->addMenu(semenu);

    QMenu *scrollmenu = new QMenu(tr("Scroll Sheet..."));
    appendSubMenu(scrollmenu, A_AUP, tr("Scroll Up (mousewheel)\tPGUP"));
    appendSubMenu(scrollmenu, A_AUP, tr("Scroll Up (mousewheel)\tALT+UP"));
    appendSubMenu(scrollmenu, A_ADOWN, tr("Scroll Down (mousewheel)\tPGDN"));
    appendSubMenu(scrollmenu, A_ADOWN, tr("Scroll Down (mousewheel)\tALT+DOWN"));
    appendSubMenu(scrollmenu, A_ALEFT, tr("Scroll Left\tALT+LEFT"));
    appendSubMenu(scrollmenu, A_ARIGHT, tr("Scroll Right\tALT+RIGHT"));

    QMenu *filtermenu = new QMenu(tr("Filter..."));
    appendSubMenu(filtermenu, A_FILTEROFF, tr("Turn filter &off"));
    appendSubMenu(filtermenu, A_FILTERS, tr("Show only cells in current search"));
    appendSubMenu(filtermenu, A_FILTER5, tr("Show 5% of last edits"));
    appendSubMenu(filtermenu, A_FILTER10, tr("Show 10% of last edits"));
    appendSubMenu(filtermenu, A_FILTER20, tr("Show 20% of last edits"));
    appendSubMenu(filtermenu, A_FILTER50, tr("Show 50% of last edits"));
    appendSubMenu(filtermenu, A_FILTERM, tr("Show 1% more than the last filter"));
    appendSubMenu(filtermenu, A_FILTERL, tr("Show 1% less than the last filter"));

    QMenu *viewmenu = new QMenu(tr("&View"));
    appendSubMenu(viewmenu, A_ZOOMIN, tr("Zoom &In (CTRL+mousewheel)\tCTRL+PGUP"));
    appendSubMenu(viewmenu, A_ZOOMOUT, tr("Zoom &Out (CTRL+mousewheel)\tCTRL+PGDN"));
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
    ui->menubar->addMenu(viewmenu);

    QMenu *roundmenu = new QMenu(tr("&Roundness of grid borders..."));
//    roundmenu->AppendRadioItem(A_ROUND0, tr("Radius &0"));
//    roundmenu->AppendRadioItem(A_ROUND1, tr("Radius &1"));
//    roundmenu->AppendRadioItem(A_ROUND2, tr("Radius &2"));
//    roundmenu->AppendRadioItem(A_ROUND3, tr("Radius &3"));
//    roundmenu->AppendRadioItem(A_ROUND4, tr("Radius &4"));
//    roundmenu->AppendRadioItem(A_ROUND5, tr("Radius &5"));
//    roundmenu->AppendRadioItem(A_ROUND6, tr("Radius &6"));
//    roundmenu->Check(sys->roundness + A_ROUND0, true);

    QMenu *optmenu = new QMenu(tr("&Options"));
    appendSubMenu(optmenu, A_DEFFONT, tr("Pick Default Font..."));
    appendSubMenu(optmenu, A_CUSTKEY, tr("Change a key binding..."));
    appendSubMenu(optmenu, A_CUSTCOL, tr("Pick Custom &Color..."));
    appendSubMenu(optmenu, A_COLCELL, tr("&Set Custom Color From Cell BG"));
    appendSubMenu(optmenu, A_DEFBGCOL, tr("Pick Document Background..."));
    optmenu->addSeparator();
//    optmenu->AppendCheckItem(A_SHOWSBAR, tr("Show Statusbar"));
//    optmenu->Check(A_SHOWSBAR, showsbar);
//    optmenu->AppendCheckItem(A_SHOWTBAR, tr("Show Toolbar"));
//    optmenu->Check(A_SHOWTBAR, showtbar);
//    optmenu->AppendCheckItem(A_LEFTTABS, tr("File Tabs on the bottom"));
//    optmenu->Check(A_LEFTTABS, lefttabs);
//    optmenu->AppendCheckItem(A_TOTRAY, tr("Minimize to tray"));
//    optmenu->Check(A_TOTRAY, sys->totray);
//    optmenu->AppendCheckItem(A_MINCLOSE, tr("Minimize on close"));
//    optmenu->Check(A_MINCLOSE, sys->minclose);
//    optmenu->AppendCheckItem(A_SINGLETRAY, tr("Single click maximize from tray"));
//    optmenu->Check(A_SINGLETRAY, sys->singletray);
    optmenu->addSeparator();
//    optmenu->AppendCheckItem(A_ZOOMSCR, tr("Swap mousewheel scrolling and zooming"));
//    optmenu->Check(A_ZOOMSCR, sys->zoomscroll);
//    optmenu->AppendCheckItem(A_THINSELC, tr("Navigate in between cells with cursor keys"));
//    optmenu->Check(A_THINSELC, sys->thinselc);
    optmenu->addSeparator();
//    optmenu->AppendCheckItem(A_MAKEBAKS, tr("Create .bak files"));
//    optmenu->Check(A_MAKEBAKS, sys->makebaks);
//    optmenu->AppendCheckItem(A_AUTOSAVE, tr("Autosave to .tmp"));
//    optmenu->Check(A_AUTOSAVE, sys->autosave);
//    optmenu->AppendCheckItem(
//        A_FSWATCH, tr("Auto reload documents"),
//        tr("Reloads when another computer has changed a file (if you have made changes, asks)"));
//    optmenu->Check(A_FSWATCH, sys->fswatch);
//    optmenu->AppendCheckItem(A_AUTOEXPORT, tr("Automatically export a .html on every save"));
//    optmenu->Check(A_AUTOEXPORT, sys->autohtmlexport);
    optmenu->addSeparator();
//    optmenu->AppendCheckItem(A_CENTERED, tr("Render document centered"));
//    optmenu->Check(A_CENTERED, sys->centered);
//    optmenu->AppendCheckItem(A_FASTRENDER, tr("Faster line rendering"));
//    optmenu->Check(A_FASTRENDER, sys->fastrender);
//    optmenu->AppendCheckItem(A_ICONSET, tr("Black and white toolbar icons"));
//    optmenu->Check(A_ICONSET, iconset);
    optmenu->addMenu(roundmenu);
    ui->menubar->addMenu(optmenu);





}
