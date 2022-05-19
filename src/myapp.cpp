#include "myapp.h"
#include "mainwindow.h"
#include "symdef.h"
#include "tools.h"
#include "mylog.h"
#include "server.h"
#include "config.h"
#include "history.h"
#include "document.h"
#include "cell.h"
#include "widget.h"
#include "grid.h"

#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QDateTime>
#include <QBuffer>
#include <QFileInfo>
#include <QDomDocument>
#include <QDomElement>
#include <QtEndian>

// 全局变量
MyApp myApp;

//! 加载示例、教程
void MyApp::loadTut()
{
    const auto &lang = QLocale::system().name().left(2);
    QString fn = Tools::resolvePath(QStringLiteral("examples/tutorial-%1.cts").arg(lang), false);
    if (!myApp.loadDB(fn, false).isEmpty())
    {
        myApp.loadDB(Tools::resolvePath(QStringLiteral("examples/tutorial.cts"), false), false);
    }
}

void MyApp::loadOpRef()
{
    myApp.loadDB(Tools::resolvePath(QStringLiteral("examples/operation-reference.cts"), false), false);
}


// 启动时初始化打开文档
static void docInit(const QString &filename)
{
    // 启动请求打开文档
    if (!filename.isEmpty())
    {
        myApp.loadDB(filename, false);
    }

    if (!myApp.frame->nb->count())
    {
        const auto &fs = myApp.fhistory->getOpenFiles();
        foreach (const QString &fn, fs)
        {
            const auto &rs = myApp.loadDB(fn, false);
            if (!rs.isEmpty()) qDebug() << fn << rs;
        }
    }

    if (!myApp.frame->nb->count()) MyApp::loadTut();

    if (!myApp.frame->nb->count()) myApp.initDB(10);


    // ScriptInit(frame);
}

// 初始化
bool MyApp::init()
{
    QString filename;
    // 是否以便携启动
    bool portable = false;
    // 是否单例程序模式的方式启动
    bool single_instance = true;

    foreach (const QString &str, QApplication::arguments().mid(1))
    {
        if (str.startsWith(QChar::fromLatin1('-')))
        {
            const char ch = str.length() > 1? str.at(1).toLatin1(): 0;
            if (ch == 'p') portable = true;
            else if (ch == 'i') single_instance = false;
        }
        else
        {
            filename = str;
        }
    }

    // 单例模式处理
    bool isok;
    server = new Server(!single_instance, &isok);
    if (single_instance && !isok)
    {
        const QString msg = filename.isEmpty() ?
                    QString::fromLatin1("*"):
                    filename;
        server->sendMsgToAnother(msg);
        qDebug() << "Instance already running, sendMsg:" << msg;
        return false;
    }

    // 日志
    log = new MyLog;

    // 翻译
    loadTranslation("ts");

    // 配置文件
    cfg = new Config(portable);

    fhistory = new FileHistory;

    // [.frame]内部赋值，初始化过程中提前用到
    new MainWindow;

    imageRef = new ImagesRef;

    docInit(filename);

    return true;
}

void MyApp::relese()
{
    DELPTR(imageRef);
    DELPTR(frame);
    DELPTR(fhistory);
    DELPTR(cfg);
    DELPTR(log);
    DELPTR(server);
}

static QByteArray uncompressNodeData(const QByteArray &data)
{
    if (data.isEmpty()) return QByteArray();
    QByteArray tmp = QByteArray(4, Qt::Uninitialized) + data;
    qToBigEndian<qint32>(data.size() * 8, (uchar*)tmp.data());
    return qUncompress(tmp);
}

// 加载本地文件
QString MyApp::loadDB(const QString &filename, bool fromreload)
{
    QString fn = filename;
    bool loadedfromtmp = false;

    if (!fromreload)
    {
        //"this file is already loaded";
        if (frame->getTabByFileName(filename)) return QString();
        const QString tfn = Tools::tmpName(filename);
        if (QFile::exists(tfn))
        {
            int ret = QMessageBox::question(
                        frame,
                        tr("Autosave load"),
                        tr("A temporary autosave file exists, would you like to load it instead?"),
                        QMessageBox::Yes, QMessageBox::No);
            if (ret == QMessageBox::Yes)
            {
                fn = tfn;
                loadedfromtmp = true;
            }
        }
    }

    bool anyimagesfailed = false;
    Document *doc = nullptr;
    QVariantMap info;

    {  // limit destructors
        QFile dfile(fn);
        if (!dfile.open(QIODevice::ReadOnly))
            return tr("Cannot open file.");

        Tools::DataIO fis(&dfile);
        char buf[4];
        quint8 version;
        fis.read(buf, 4);
        if (strncmp(buf, "TSFF", 4))
            return tr("Not a TreeSheets file.");

        fis.read((char*)&version, 1);
        if (version > TS_VERSION)
            return tr("File of newer version.");

        info[QStringLiteral("version")] = int (version);
        info[QStringLiteral("fakelasteditonload")] = QDateTime::currentMSecsSinceEpoch();

        Images imgs;
        forever
        {
            fis.read(buf, 1);
            if (*buf == 'I')
            {
                if (version < 9) fis.readString();
                double sc = version >= 19 ? fis.readDouble() : 1.0;

                QImage im;
                off_t beforepng = fis.pos();
                bool ok = im.load(fis.d(), nullptr);
                if (!ok)
                {
                    // Uhoh.. the decoder failed. Try to save the situation by skipping this
                    // PNG.
                    anyimagesfailed = true;
                    if (beforepng <= 0) return tr("Cannot tell/seek document?");
                    fis.seek(beforepng);
                    // Now try to skip past this PNG
                    char header[8];
                    fis.read(header, 8);
                    uchar expected[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
                    if (memcmp(header, expected, 8)) return tr("Corrupt PNG header.");
                    fis.setByteOrder(DIO_BO_BIG);
                    for (;;)  // Skip all chunks.
                    {
                        qint32 len = fis.readInt32();
                        char fourcc[4];
                        fis.read(fourcc, 4);
                        fis.seek(len + fis.pos());  // skip data
                        fis.readInt32();                   // skip CRC
                        if (memcmp(fourcc, "IEND", 4) == 0) break;
                    }
                    fis.setByteOrder(DIO_BO_Little);
                    // Set empty image here, since document expect there to be one here.
                    int sz = 32;
                    im = QImage(sz, sz, QImage::Format_RGB888);
                    im.fill(Qt::red);
                }
                imgs << wrapImage(Image(im, sc));
            }
            else if (*buf == 'D')
            {
                QByteArray databuff = uncompressNodeData(fis.readAll());
                if (!databuff.size()) return tr("Cannot decompress file.");
                QBuffer dbuff(&databuff);
                dbuff.open(QIODevice::ReadOnly);
                Tools::DataIO dis(&dbuff);

                info[QStringLiteral("imgs")] = QVariant::fromValue(imgs);
                int numcells = 0, textbytes = 0;
                Cell *root = Cell::loadWhich(dis, nullptr, numcells, textbytes, info);
                if (!root) return tr("File corrupted!");

                Widget *widget = frame->createWidget(true);
                doc = widget->doc;
                if (loadedfromtmp)
                {
                    // if not, user will lose tmp without warning when he closes
                    doc->undolistsizeatfullsave = -1;
                    doc->modified = true;
                }
                doc->initWith(root, filename);

                if (version >= 11)
                {
                    forever
                    {
                        const QString &s = dis.readString();
                        if (s.isEmpty()) break;
                        doc->tags.append(s);
                    }
                }

                doc->sw->status(tr("Loaded %3 (%1 cells, %2 characters).")
                                .arg(numcells).arg(textbytes).arg(filename));
                break;
            }
            else
            {
                return tr("Corrupt block header.");
            }
        }
    }
    fileUsed(filename, doc);

    doc->clearSelectionRefresh();

    if (anyimagesfailed)
    {
        QMessageBox::information(frame, tr("PNG decoder failure"),
                                 tr("PNG decode failed on some images in this document\nThey have been replaced by red squares."));
    }
    return QString();
}

Cell *&MyApp::initDB(int sizex, int sizey)
{
    Cell *c = new Cell(nullptr, nullptr, CT_DATA, new Grid(sizex, sizey ? sizey : sizex));
    c->cellcolor = 0xCCDCE2;
    c->grid->initCells();
    Widget*widget = myApp.frame->createWidget(false);
    Document *doc = widget->doc;
    doc->initWith(c, QString());
    return doc->rootgrid;
}

ImagePtr MyApp::wrapImage(const Image &img)
{
    auto itr = imageRef->begin();
    while (itr != imageRef->end())
    {
        auto i = itr->data();
        if (i == nullptr)
        {
            itr = imageRef->erase(itr);
        }
        else if (img == *i)
        {
            return itr->toStrongRef();
        }
        else
        {
            itr++;
        }
    }
    ImagePtr ret(new Image(img));
    imageRef->append(ret.toWeakRef());
    return ret;
}

void MyApp::fileUsed(const QString &filename, Document *doc)
{
    fhistory->addFileToHistory(filename);
    fhistory->rememberOpenFiles();
    if (cfg->fswatch)
    {
        QFileInfo fi = QFileInfo(filename);
        doc->lastmodificationtime = fi.lastModified();
        const QString &d = fi.absoluteFilePath();
        frame->fileChangeWatch(d);
    }
}

QString MyApp::open(const QString &fn)
{
    if (!fn.isEmpty())
    {
        const QString &msg = loadDB(fn, false);
        if (!msg.isEmpty())
        {
            QMessageBox::information(frame, fn, msg);
        }
        return msg;
    }
    return tr("Open file cancelled.");
}

static void fillXML(Cell *c, const QDomElement &n)
{
    if (n.nodeName() != "cell") return;
    if (n.hasAttribute("relsize")) c->text.relsize = -n.attribute("relsize").toInt();
    if (n.hasAttribute("stylebits")) c->text.stylebits = n.attribute("stylebits").toInt();
    if (n.hasAttribute("colorbg")) c->cellcolor = n.attribute("colorbg").toInt();
    if (n.hasAttribute("colorfg")) c->textcolor = n.attribute("colorfg").toInt();
    if (n.hasAttribute("type")) c->celltype = n.attribute("type").toInt();
    if (n.hasAttribute("vertical")) c->verticaltextandgrid = n.attribute("vertical").toInt();

    auto children = n.childNodes();
    QDomNode grid;
    for (int i = 0; i < children.size(); i++)
    {
        auto child = children.at(i);
        if (child.nodeType() == QDomNode::TextNode)
        {
            if (c->text.t.size()) c->text.t.append(' ');
            c->text.t += child.nodeValue().trimmed();
        }
        else if (child.nodeType() == QDomNode::ElementNode && child.nodeName() == "grid")
        {
            grid = child;
        }
    }

    if (grid.isNull()) return;
    const auto rows = grid.childNodes();
    int rownum = rows.size();
    int colnum = -1;
    for (int y = 0; y < rownum; y++)
    {
        if (rows.at(y).nodeType() != QDomNode::ElementNode || rows.at(y).nodeName() != "row") continue;
        const auto row = rows.at(y).childNodes();
        if (colnum == -1)
        {
            colnum = row.size();
            if (colnum == 0) break;
            c->addGrid(colnum, rownum);
        }
        for (int x = 0, e = qMin(colnum, row.size()); x < e; x++)
        {
            auto cell = row.at(x);
            if (cell.nodeType() == QDomNode::ElementNode && cell.nodeName() == "cell")
            {
                fillXML(c->grid->C(x, y), cell.toElement());
            }
        }
    }
}

QString MyApp::import(int k)
{
    const QString fn = Dialog::openFile(
                tr("Please select file to import:"),
                "*.* (*.*)");
    if (!fn.isEmpty())
    {
        switch (k) {
        case A_IMPXML:
        /*case A_IMPXMLA:*//* 无法理解 OPML 该是什么样的，第三方库的差异，也让人一头雾水，决定暂时放弃该类型 */
        {
            QDomDocument doc;
            QFile file(fn);
            if (!file.open(QIODevice::ReadOnly)) goto problem;
            if (!doc.setContent(&file)) goto problem;
            Cell *&r = initDB(1);
            Cell *c = *r->grid->cells;
            fillXML(c, doc.documentElement());
            if (!c->hasText() && c->grid)
            {
                *r->grid->cells = nullptr;
                delete r;
                r = c;
                c->p = nullptr;
            }
            break;
        }
        case A_IMPTXTI:
        case A_IMPTXTC:
        case A_IMPTXTS:
        case A_IMPTXTT: {
            QFile f(fn);
            if (!f.open(QIODevice::ReadOnly)) goto problem;
            const QString &s =  QString::fromUtf8(f.readAll());
            if (!s.isEmpty()) goto problem;
            const QStringList &as = s.split(LINE_SEPERATOR);
            if (as.size())
            {
                switch (k)
                {
                case A_IMPTXTI: {
                    Cell *r = initDB(1);
                    Grid::fillRows(r->grid, as, Tools::countCol(as[0]), 0, 0);
                    break;
                }
                case A_IMPTXTC: initDB(1, as.size())->grid->CSVImport(as, ','); break;
                case A_IMPTXTS: initDB(1, as.size())->grid->CSVImport(as, ';'); break;
                case A_IMPTXTT: initDB(1, as.size())->grid->CSVImport(as, '\t'); break;
                }
            }
            break;
        }
        }
        frame->getCurTab()->doc->changeFileName(fn.indexOf('.') >= 0 ? fn.section('.', 0, -2) : fn, true);
        frame->getCurTab()->doc->clearSelectionRefresh();
    }
    return QString();
problem:
    QMessageBox::warning(frame, fn, tr("couldn't import file!"));
    return tr("File load error.");
}

// 加载翻译文件
void MyApp::loadTranslation(const QString &filename)
{
    const QString basepath = Tools::resolvePath(TR_FILEPATH, true);
    if (basepath.isEmpty()) return;

    QTranslator *translator = new QTranslator;
    if (translator->load(QLocale::system(), filename, "_", basepath, ".qm"))
    {
        QApplication::installTranslator(translator);
        qDebug() << "load translator: " << filename << QLocale::system().name();
    }
    else
    {
        delete translator;
    }
}
