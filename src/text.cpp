#include "text.h"
#include "tools.h"
#include "selection.h"
#include "config.h"
#include "cell.h"
#include "grid.h"
#include "myapp.h"
#include "mainwindow.h"
#include "document.h"

#include <QVariantMap>
#include <QPainter>
#include <QDebug>

void Text::load(Tools::DataIO &dis, QVariantMap &info)
{
    t = dis.readString();

    // if (t.length() > 10000)
    //    printf("");

    int version = info.value(QStringLiteral("version")).toInt();
    if (version <= 11) dis.readInt32();  // numlines

    relsize = dis.readInt32();

    int i = dis.readInt32();
    if (i >= 0)
    {
        const Images &imgs = info.value(QStringLiteral("imgs")).value<Images>();
        Q_ASSERT(i < imgs.size());
        image = imgs.value(i);
    }
    else
    {
        image.clear();
    }

    if (version >= 7) stylebits = dis.readInt32();

    qint64 time;
    if (version >= 14)
    {
        time = dis.readInt64();
    }
    else
    {
        const QString &k = QStringLiteral("fakelasteditonload");
        time = info.value(k).toLongLong();
        info[k] = time - 1;
    }
    lastedit = QDateTime::fromMSecsSinceEpoch(time);
}

bool Text::isInSearch() const
{
    return myApp.frame->searchstring.length() &&
            t.toLower().indexOf(myApp.frame->searchstring) >= 0;
}

void Text::selectWord(Selection &s) const
{
    if (s.cursor >= t.length()) return;
    s.cursorend = s.cursor + 1;
    expandToWord(s);
}

void Text::expandToWord(Selection &s) const
{
    if (!t.at(s.cursor).isLetterOrNumber()) return;
    while (s.cursor > 0 && t.at(s.cursor - 1).isLetterOrNumber()) s.cursor--;
    while (s.cursorend < t.length() && t.at(s.cursorend).isLetterOrNumber()) s.cursorend++;
}

bool Text::isWord(const QChar &c) const
{
    return c.isLetterOrNumber() || QStringLiteral("_\"\'()").indexOf(c) != -1;
}

QString Text::getLinePart(int &i, int p, int l) const
{
    int start = i;
    i = p;

    while (i < l && t[i] != QChar::fromLatin1(' ') && !isWord(t[i]))
    {
        i++;
        p++;
    };  // gobble up any trailing punctuation
    if (i != start && i < l && (t[i] == '\"' || t[i] == '\''))
    {
        i++;
        p++;
    }  // special case: if punctuation followed by quote, quote is meant to be part of word

    while (i < l && t[i] == L' ')  // gobble spaces, but do not copy them
    {
        i++;
        if (i == l)
            p = i;  // happens with a space at the last line, user is most likely about to type
        // another word, so
        // need to show space. Alternatively could check if the cursor is actually on this spot.
        // Simply
        // showing a blank new line would not be a good idea, unless the cursor is here for
        // sure, and
        // even then, placing the cursor there again after deselect may be hard.
    }

    Q_ASSERT(start != i);

    return t.mid(start, p - start);
}

QString Text::getLine(int &i, int maxcolwidth) const
{
    int l = t.length();

    if (i >= l) return QString();

    if (!i && l <= maxcolwidth)
    {
        i = l;
        return t;
    }  // subsumed by the case below, but this case happens 90% of the time, so more optimal
    if (l - i <= maxcolwidth) return getLinePart(i, l, l);

    for (int p = i + maxcolwidth; p >= i; p--)
        if (!isWord(t[p])) return getLinePart(i, p, l);

    // A single word is > maxcolwidth. We split it up anyway.
    // This happens with long urls and e.g. Japanese text without spaces.
    // Should really do proper unicode linebreaking instead (see libunibreak),
    // but for now this is better than the old code below which allowed for arbitrary long
    // words.
    return getLinePart(i, qMin(i + maxcolwidth, l), l);

    // for(int p = i+maxcolwidth; p<l;  p++) if(!IsWord(t[p])) return GetLinePart(i, p, l);  //
    // we arrive here only
    // if a single word is too big for maxcolwidth, so simply return that word
    // return GetLinePart(i, l, l);     // big word was the last one
}

void Text::relSize(int dir, int zoomdepth)
{
    const int minv = _g::deftextsize - _g::mintextsize() + zoomdepth;
    const int maxv = _g::deftextsize - _g::maxtextsize() - zoomdepth;
    relsize = qMax(qMin(relsize + dir, minv), maxv);
}

QImage Text::displayImage()
{
    ImagePtr ptr = cell->grid && cell->grid->folded
            ? myApp.frame->foldicon : image;
    return ptr.data()? ptr->display(): QImage();
}

void Text::disImgSize(int &xs, int &ys)
{
    const auto &img = displayImage();
    if (!img.isNull())
    {
        xs = img.width();
        ys = img.height();
    }
}

void Text::textSize(QPainter &dc, int &sx, int &sy, bool tiny, int &leftoffset, int maxcolwidth)
{
    sx = sy = 0;
    int i = 0;
    forever
    {
        const QString &curl = getLine(i, maxcolwidth);
        if (!curl.length()) break;
        int x, y;
        if (tiny)
        {
            x = curl.length();
            y = 1;
        }
        else
        {
            const QSize &s = dc.fontMetrics().size(Qt::TextSingleLine, curl);
            x = s.width();
            y = s.height();
        }
        sx = qMax(x, sx);
        sy += y;
        leftoffset = y;
    }
    if (!tiny) sx += 4;
}

static void myDrawText(QPainter &dc, const QString &s, int x, int y, int w, int h)
{
#ifdef __WXMSW__  // this special purpose implementation is because the MSW implementation calls
    // TextExtent, which costs
    // 25% of all cpu time
    dc.CalcBoundingBox(x, y);
    dc.CalcBoundingBox(x + w, y + h);
    HDC hdc = (HDC)dc.GetHDC();
    ::SetTextColor(hdc, dc.GetTextForeground().GetPixel());
    ::SetBkColor(hdc, dc.GetTextBackground().GetPixel());
    ::ExtTextOut(hdc, x, y, 0, nullptr, s.c_str(), s.length(), nullptr);
#else
    dc.drawText(dc.boundingRect(x, y, w, h, Qt::AlignLeft | Qt::AlignVCenter, s), s);
#endif
}

int Text::render(Document *doc, int bx, int by, int depth, QPainter &dc, int &leftoffset, int maxcolwidth)
{
    int ixs = 0, iys = 0;
    if (!cell->tiny)
    {
        disImgSize(ixs, iys);
    }
    if (ixs && iys)
    {
        dc.drawImage(bx + 1 + _g::margin_extra, by + (cell->tys - iys) / 2 + _g::margin_extra, displayImage());
        ixs += 2;
        iys += 2;
    }

    if (t.isEmpty()) return iys;

    doc->pickFont(dc, depth, relsize, stylebits);

    // TODO
    int h = cell->tiny ? 1 : dc.fontMetrics().height();
    leftoffset = h;
    int i = 0;
    int lines = 0;
    bool searchfound = isInSearch();
    bool istag = cell->isTag(doc);

    if (searchfound) dc.setPen(QColor(Qt::red));
    else if (filtered) dc.setPen(QColor(Qt::lightGray));
    else if (istag) dc.setPen(QColor(Qt::blue));
    else if (cell->tiny) dc.setPen(Color(myApp.cfg->pen_tinytext));
    else if (cell->textcolor) dc.setPen(Color(cell->textcolor)); // FIXME: clean up
    else dc.setPen(QColor(Qt::black));

    forever
    {
        const QString &curl = getLine(i, maxcolwidth);
        if (curl.isEmpty()) break;
        if (cell->tiny)
        {
            if (myApp.cfg->fastrender)
            {
                dc.drawLine(bx + ixs, by + lines * h, bx + ixs + curl.length(), by + lines * h);
            }
            else
            {
                int word = 0;
                for (int p = 0; p < curl.length() + 1; p++)
                {
                    if (curl.length() <= p || curl[p] == QChar::fromLatin1(' '))
                    {
                        if (word)
                        {
                            dc.drawLine(bx + p - word + ixs, by + lines * h,
                                              bx + p, by + lines * h);
                        }
                        word = 0;
                    }
                    else word++;
                }
            }
        }
        else
        {
            int tx = bx + 2 + ixs;
            int ty = by + lines * h;
            myDrawText(dc, curl, tx + _g::margin_extra, ty + _g::margin_extra, cell->sx, h);
        }
        lines++;
    }

    return qMax(lines * h, iys);
}

void Text::findCursor(Document *doc, int bx, int by, QPainter &dc, Selection &s, int maxcolwidth)
{
    bx -= _g::margin_extra;
    by -= _g::margin_extra;

    int ixs = 0, iys = 0;
    if (!cell->tiny) disImgSize(ixs, iys);
    if (ixs) ixs += 2;

    doc->pickFont(dc, cell->depth() - doc->drawpath.size(), relsize, stylebits);

    int i = 0, linestart = 0;
    int line = by / dc.fontMetrics().height();
    QString ls;

    for (int l = 0; l < line + 1; l++)
    {
        linestart = i;
        ls = getLine(i, maxcolwidth);
    }

    forever
    {
        const QSize &s = dc.fontMetrics().size(Qt::TextSingleLine, ls);
        if (s.width() <= bx - ixs + 2 || !s.width()) break;
        ls.resize(ls.length() - 1);
    }

    s.cursor = s.cursorend = linestart + ls.length();
    Q_ASSERT(s.cursor >= 0 && s.cursor <= t.length());
}
