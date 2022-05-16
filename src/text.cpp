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

int Text::render(Document *doc, int bx, int by, int depth, QPainter &dc, int &leftoffset, int maxcolwidth)
{
    int ixs = 0, iys = 0;
    if (!cell->tiny)
    {
        disImgSize(ixs, iys);
    }
    if (ixs && iys)
    {
        Tools::drawImage(dc, bx + 1 + _g::margin_extra, by + (cell->tys - iys) / 2 + _g::margin_extra, displayImage());
        ixs += 2;
        iys += 2;
    }

    if (t.isEmpty()) return iys;

    doc->pickFont(dc, depth, relsize, stylebits);

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
                Tools::drawLine(dc, bx + ixs, by + lines * h, bx + ixs + curl.length(), by + lines * h);
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
                            Tools::drawLine(dc, bx + p - word + ixs, by + lines * h,
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
            Tools::drawText(dc, curl, tx + _g::margin_extra, ty + _g::margin_extra, cell->sx, h);
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

void Text::insert(Document *doc, const QString &ins, Selection &s)
{
    auto prevl = t.length();
    if (!s.textEdit()) clear(doc, s);
    rangeSelRemove(s);
    if (!prevl) setRelSize(s);
    t.insert(s.cursor, ins);
    s.cursor = s.cursorend = s.cursor + ins.length();
}

void Text::clear(Document *doc, Selection &s)
{
    t.clear();
    s.enterEdit(doc);
}

bool Text::rangeSelRemove(Selection &s)
{
    wasEdited();
    if (s.cursor != s.cursorend)
    {
        t.remove(s.cursor, s.cursorend - s.cursor);
        s.cursorend = s.cursor;
        return true;
    }
    return false;
}

void Text::setRelSize(Selection &s)
{
    if (t.length() || !cell->p) return;
    int dd[] = { 0, 1, 1, 0, 0, -1, -1, 0 };
    for (int i = 0; i < 4; i++)
    {
        int x = qMax(0, qMin(s.x + dd[i * 2], s.g->xs - 1));
        int y = qMax(0, qMin(s.y + dd[i * 2 + 1], s.g->ys - 1));
        auto c = s.g->C(x, y);
        if (c->text.t.length())
        {
            relsize = c->text.relsize;
            break;
        }
    }
}

void Text::drawCursor(Document *doc, QPainter &dc, Selection &s, uint color, bool cursoronly, int maxcolwidth)
{
    int ixs = 0, iys = 0;
    if (!cell->tiny) disImgSize(ixs, iys);
    if (ixs) ixs += 2;
    doc->pickFont(dc, cell->depth() - doc->drawpath.size(), relsize, stylebits);
    const auto fm = dc.fontMetrics();
    int h = fm.height();
    {
        int i = 0;
        for (int l = 0;; l++) {
            int start = i;
            QString ls = getLine(i, maxcolwidth);
            int len = ls.length();
            int end = start + len;

            if (s.cursor != s.cursorend)
            {
                if (s.cursor <= end && s.cursorend >= start && !cursoronly)
                {
                    ls.resize(qMin(s.cursorend, end) - start);
                    int x1, x2;
                    x2 = fm.size(Qt::TextSingleLine, ls).width();
                    ls.resize(qMax(s.cursor, start) - start);
                    x1 = fm.size(Qt::TextSingleLine, ls).width();
                    if (x1 != x2)
                    {
                        doc->cursorlastinfo.setRect(
                                    cell->getX(doc) + x1 + 2 + ixs + _g::margin_extra,
                                    cell->getY(doc) + l * h + 1 + cell->ycenteroff + _g::margin_extra,
                                    x2 - x1, h - 1);
                        Tools::drawRect(
                                    dc, color,
                                    doc->cursorlastinfo.x(),
                                    doc->cursorlastinfo.y(),
                                    doc->cursorlastinfo.width(),
                                    doc->cursorlastinfo.height()
            #ifdef SIMPLERENDER
                                    ,
                                    true
            #endif
                                    );
                    }
                }
            }
            else if (s.cursor >= start && s.cursor <= end)
            {
                ls.resize(s.cursor - start);
                int x = fm.size(Qt::TextSingleLine, ls).width();
                if (doc->blink)
                {
#ifndef SIMPLERENDER
                    // It will blink this on/off with xwXOR set in the caller.
                    color = 0xFFFFFF;
#endif
                    doc->cursorlastinfo.setRect(
                                cell->getX(doc) + x + 1 + ixs + _g::margin_extra,
                                cell->getY(doc) + l * h + 1 + cell->ycenteroff + _g::margin_extra,
                                2, h - 2);
                    Tools::drawRect(
                                dc, color,
                                doc->cursorlastinfo.x(),
                                doc->cursorlastinfo.y(),
                                doc->cursorlastinfo.width(),
                                doc->cursorlastinfo.height());
                }
                break;
            }

            if (!len) break;
        }
    }
}
