#include "myapp.h"
#include "mainwindow.h"
#include "symdef.h"
#include "tools.h"
#include "mylog.h"
#include "server.h"
#include "config.h"
#include "history.h"
#include "document.h"

#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QDataStream>
#include <QDateTime>
#include <cmath>
#include <QtEndian>

// 全局变量
MyApp myApp;

//! 加载示例、教程
static void LoadTut()
{
    const auto &lang = QLocale::system().name().left(2);
    QString fn = Tools::resolvePath(QStringLiteral("examples/tutorial-%1.cts").arg(lang), false);
    if (!myApp.LoadDB(fn, false).isEmpty())
    {
        myApp.LoadDB(Tools::resolvePath(QStringLiteral("examples/tutorial.cts"), false), false);
    }
}

// 启动时初始化打开文档
static void docInit(const QString &filename)
{
    // 启动请求打开文档
    if (!filename.isEmpty())
    {
        myApp.LoadDB(filename, false);
    }

    if (!myApp.frame->nb->count())
    {
        int numfiles = myApp.cfg->read(QStringLiteral("numopenfiles"), 0).toInt();
        for (int i = 0; i < numfiles; i++)
        {
            QString fn = QStringLiteral("lastopenfile_%d").arg(i);
            fn = myApp.cfg->read(fn, i).toString();
            if (!fn.isEmpty()) myApp.LoadDB(fn, false);
        }
    }

    if (!myApp.frame->nb->count()) LoadTut();

    if (!myApp.frame->nb->count()) myApp.InitDB(10);


    // ScriptInit(frame);
}

class File : public QFile
{
public:
    File(const QString &name) : QFile(name) {}
    QString readString()
    {
        quint16 len = 0;
        if (read((char*)&len, 2) != 2) return QString();
        QByteArray d = read(len);
        if (d.size() != len) return QString();
        return QString::fromUtf8(d);
    }
    qint32 readInt32()
    {
        char buff[4];
        if (read(buff, 4) != 4) return 0;
        return qFromBigEndian<qint32>((uchar*)buff);
    }
    double readDouble()
    {
        char bytes[10];
        if (read(bytes, 10) != 10) return 0.;

        double f;
        qint32 expon;
        quint32 hiMant, loMant;

        expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
        hiMant = ((quint32)(bytes[2] & 0xFF) << 24)
                | ((quint32)(bytes[3] & 0xFF) << 16)
                | ((quint32)(bytes[4] & 0xFF) << 8)
                | ((quint32)(bytes[5] & 0xFF));
        loMant = ((quint32)(bytes[6] & 0xFF) << 24)
                | ((quint32)(bytes[7] & 0xFF) << 16)
                | ((quint32)(bytes[8] & 0xFF) << 8)
                | ((quint32)(bytes[9] & 0xFF));

        if (expon == 0 && hiMant == 0 && loMant == 0)
        {
            f = 0;
        }
        else
        {
            if (expon == 0x7FFF)
            { /* Infinity or NaN */
                f = HUGE_VAL;
            }
            else
            {
                expon -= 16383;
                f  = ldexp(double (hiMant), expon-=31);
                f += ldexp(double (loMant), expon-=32);
            }
        }
        return (bytes[0] & 0x80)? -f: f;
    }
};

// 初始化
bool MyApp::Init()
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

void MyApp::Relese()
{
    DELPTR(imageRef);
    DELPTR(frame);
    DELPTR(fhistory);
    DELPTR(cfg);
    DELPTR(log);
    DELPTR(server);
}

// 加载本地文件
QString MyApp::LoadDB(const QString &filename, bool fromreload)
{
    // TODO
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


//    Document *doc = nullptr;
    bool anyimagesfailed = false;

    {  // limit destructors
        File fis(fn);
        if (!fis.open(QIODevice::ReadOnly)) return tr("Cannot open file.");
        char buf[4];
        quint8 version;
        fis.read(buf, 4);
        if (strncmp(buf, "TSFF", 4)) return tr("Not a TreeSheets file.");
        fis.read((char*)&version, 1);
        if (version > TS_VERSION) return tr("File of newer version.");

        // auto fakelasteditonload = QDateTime::currentMSecsSinceEpoch();

        Images imgs;
        for (;;) {
            fis.read(buf, 1);
            switch (*buf) {
                case 'I': {
                    if (version < 9) fis.readString();
                    double sc = version >= 19 ? fis.readDouble() : 1.0;

                    QImage im;
                    off_t beforepng = fis.pos();
                    bool ok = im.load(&fis, "PNG");

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
                        for (;;)  // Skip all chunks.
                        {
                            qint32 len = fis.readInt32();
                            char fourcc[4];
                            fis.read(fourcc, 4);
                            fis.seek(len + fis.pos());  // skip data
                            fis.readInt32();                   // skip CRC
                            if (memcmp(fourcc, "IEND", 4) == 0) break;
                        }
                        // Set empty image here, since document expect there to be one here.
                        int sz = 32;
                        im = QImage(sz, sz, QImage::Format_RGB888);
                        im.fill(Qt::red);
                    }
                    imgs << wrapImage(Image(im, sc));
                    break;
                }

                case 'D': {
//                    wxZlibInputStream zis(fis);
                    auto databuff = fis.readAll();
                    int len = databuff.size();
                    databuff = qUncompress(QByteArray((char*)&len, 4) + databuff);
                    if (!databuff.size()) return tr("Cannot decompress file.");

//                    wxDataInputStream dis(zis);
//                    int numcells = 0, textbytes = 0;
//                    Cell *root = Cell::LoadWhich(dis, nullptr, numcells, textbytes);
//                    if (!root) return _(L"File corrupted!");

//                    doc = NewTabDoc(true);
//                    if (loadedfromtmp) {
//                        doc->undolistsizeatfullsave =
//                            -1;  // if not, user will lose tmp without warning when he closes
//                        doc->modified = true;
//                    }
//                    doc->InitWith(root, filename);

//                    if (versionlastloaded >= 11) {
//                        for (;;) {
//                            wxString s = dis.ReadString();
//                            if (!s.Len()) break;
//                            doc->tags[s] = true;
//                        }
//                    }

//                    doc->sw->Status(wxString::Format(_(L"Loaded %s (%d cells, %d characters)."),
//                                                     filename.c_str(), numcells, textbytes)
//                                        .c_str());

//                    goto done;
                }

                default: return tr("Corrupt block header.");
            }
        }
    }

//done:

//    FileUsed(filename, doc);

//    doc->ClearSelectionRefresh();

//    if (anyimagesfailed)
//        wxMessageBox(
//            _(L"PNG decode failed on some images in this document\nThey have been replaced by "
//              L"red squares."),
//            _(L"PNG decoder failure"), wxOK, frame);

//    return L"";







    return QString();
}

Cell *MyApp::InitDB(int sizex, int sizey)
{
    // TODO

    return nullptr;
//    Cell *c = new Cell(nullptr, nullptr, CT_DATA, new Grid(sizex, sizey ? sizey : sizex));
//    c->cellcolor = 0xCCDCE2;
//    c->grid->InitCells();
//    Document *doc = NewTabDoc();
//    doc->InitWith(c, L"");
    //    return doc->rootgrid;
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
