#ifndef SELECTION_H
#define SELECTION_H

struct Grid;
struct Cell;
struct Document;
class QPainter;
class QChar;

class Selection
{
public:
    Grid *g;
    int x;
    int y;
    int xs;
    int ys;
    int cursor;
    int cursorend;
    int firstdx;
    int firstdy;
private:
    bool textedit;

public:
    Selection();
    Selection(Grid *_g, int _x, int _y, int _xs, int _ys);

    void setCursorEdit(Document *doc, bool edit);
    void enterEditOnly(Document *doc);
    void enterEdit(Document *doc, int c = 0, int ce = 0);
    void exitEdit(Document *doc);
    bool isInside(const Selection &o) const;

    void selAll();
    Cell *getFirst() const;
    Cell *getCell() const;
    bool thin() const;
    bool isAll() const;
    bool eqLoc(const Selection &s) const;
    bool operator==(const Selection &s) const;
    bool textEdit() const;






    void merge(const Selection &a, const Selection &b);
    int maxCursor() const;
    void dir(Document *doc, bool ctrl, bool shift, QPainter &dc, int dx, int dy, int &v, int &vs,
             int &ovs, bool notboundaryperp, bool notboundarypar, bool exitedit);
    void Cursor(Document *doc, int k, bool ctrl, bool shift, QPainter &dc,
                bool exitedit = false);
    void next(Document *doc, QPainter &dc, bool backwards);
    const QChar *wrap(Document *doc);
    Cell *thinExpand(Document *doc);
    void homeEnd(Document *doc, QPainter &dc, bool ishome);
};


#endif // SELECTION_H
