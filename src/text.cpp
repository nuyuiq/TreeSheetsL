#include "text.h"
#include "tools.h"
#include "selection.h"

#include <QVariantMap>

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
