#ifndef TEXT_H
#define TEXT_H

#include "image.h"

#include <QString>
#include <QDateTime>
#include <QVariantMap>


struct Cell;
struct Document;
class Selection;
class QImage;
class QPainter;
namespace Tools { class DataIO; }


class Text
{
public:
    QString t;
    ImagePtr image;
    QDateTime lastedit;
    Cell *cell;
    int relsize;
    int stylebits;
    int extent;
    bool filtered;


    // 初始化
    inline Text() : cell(nullptr), relsize(0),
        stylebits(0), extent(0), filtered(false) {}

    //! 从序列化中加载数据
    void load(Tools::DataIO &dis, QVariantMap &info);
    void save(Tools::DataIO &dos, const QVector<ImagePtr> &imgs) const;
    bool isInSearch() const;
    void selectWord(Selection &s) const;
    void expandToWord(Selection &s) const;
    bool isWord(const QChar &c) const;
    QString getLinePart(int &i, int p, int l) const;
    QString getLine(int &i, int maxcolwidth) const;
    void relSize(int dir, int zoomdepth);
    QImage displayImage();
    void disImgSize(int &xs, int &ys);
    void textSize(QPainter &dc, int &sx, int &sy, bool tiny, int &leftoffset, int maxcolwidth);
    int render(Document *doc, int bx, int by, int depth, QPainter &dc, int &leftoffset, int maxcolwidth);
    void findCursor(Document *doc, int bx, int by, QPainter &dc, Selection &s, int maxcolwidth);
    void insert(Document *doc, const QString &ins, Selection &s);
    void clear(Document *doc, Selection &s);
    bool rangeSelRemove(Selection &s);
    void setRelSize(Selection &s);
    void drawCursor(Document *doc, QPainter &dc, Selection &s, uint color, bool cursoronly, int maxcolwidth);
    QString toText(int indent, const Selection &s, int format);
    void replaceStr(const QString &str);
    void backspace(Selection &s);
    void Delete(Selection &s);
    void selectWordBefore(Selection &s);
    void homeEnd(Selection &s, bool home);
    inline void key(Document *doc, const QString &ins, Selection &s)
    {
        insert(doc, ins, s);
    }


    inline void backspaceWord(Selection &s)
    {
        selectWordBefore(s);
        backspace(s);
    }
    inline void deleteWord(Selection &s)
    {
        selectWord(s);
        Delete(s);
    }
    inline int minRelsize(int rs) const { return qMin(relsize, rs); }
    //! 更新修改时间为当前
    inline void wasEdited() { lastedit = QDateTime::currentDateTime(); }

};

#endif // TEXT_H
