#ifndef TEXT_H
#define TEXT_H

#include "image.h"
#include <QString>
#include <QDateTime>


struct Cell;
class Selection;
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

    void selectWord(Selection &s) const;
    void expandToWord(Selection &s) const;
    bool isWord(const QChar &c) const;
    QString getLinePart(int &i, int p, int l) const;
    QString getLine(int &i, int maxcolwidth) const;

    //! 更新修改时间为当前
    inline void wasEdited() { lastedit = QDateTime::currentDateTime(); }

};

#endif // TEXT_H
