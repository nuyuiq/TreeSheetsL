#ifndef CONFIG_H
#define CONFIG_H

#include <QVariant>

class QSettings;
class QString;


enum {
    A_NEW = 500,
    A_OPEN,
    A_CLOSE,
    A_SAVE,
    A_SAVEAS,
    A_CUT,
    A_COPY,
    A_PASTE,
    A_NEWGRID,
    A_UNDO,
    A_ABOUT,
    A_RUN,
    A_CLRVIEW,
    A_MARKDATA,
    A_MARKVIEWH,
    A_MARKVIEWV,
    A_MARKCODE,
    A_IMAGE,
    A_EXPIMAGE,
    A_EXPXML,
    A_EXPHTMLT,
    A_EXPHTMLO,
    A_EXPHTMLB,
    A_EXPTEXT,
    A_ZOOMIN,
    A_ZOOMOUT,
    A_TRANSPOSE,
    A_DELETE,
    A_BACKSPACE,
    A_DELETE_WORD,
    A_BACKSPACE_WORD,
    A_LEFT,
    A_RIGHT,
    A_UP,
    A_DOWN,
    A_MLEFT,
    A_MRIGHT,
    A_MUP,
    A_MDOWN,
    A_SLEFT,
    A_SRIGHT,
    A_SUP,
    A_SDOWN,
    A_ALEFT,
    A_ARIGHT,
    A_AUP,
    A_ADOWN,
    A_SCLEFT,
    A_SCRIGHT,
//    A_SCUP,
//    A_SCDOWN,
    A_DEFFONT,
    A_IMPXML,
    A_IMPXMLA,
    A_IMPTXTI,
    A_IMPTXTC,
    A_IMPTXTS,
    A_IMPTXTT,
    A_HELP,
    A_MARKVARD,
    A_MARKVARU,
    A_SHOWSBAR,
    A_SHOWTBAR,
    A_LEFTTABS,
//    A_TRADSCROLL,
    A_HOME,
    A_END,
    A_CHOME,
    A_CEND,
    A_PRINT,
    A_PREVIEW,
    A_PAGESETUP,
    A_PRINTSCALE,
    A_EXIT,
    A_NEXT,
    A_PREV,
    A_BOLD,
    A_ITALIC,
    A_TT,
    A_UNDERL,
    A_SEARCH,
    A_REPLACE,
    A_REPLACEONCE,
    A_REPLACEONCEJ,
    A_REPLACEALL,
    A_SELALL,
    A_CANCELEDIT,
    A_BROWSE,
    A_ENTERCELL,
    A_PROGRESSCELL, // see https://github.com/aardappel/treesheets/issues/139#issuecomment-544167524
    A_CELLCOLOR,
    A_TEXTCOLOR,
    A_BORDCOLOR,
    A_INCSIZE,
    A_DECSIZE,
    A_INCWIDTH,
    A_DECWIDTH,
    A_ENTERGRID,
    A_LINK,
    A_LINKREV,
    A_SEARCHNEXT,
    A_CUSTCOL,
    A_COLCELL,
    A_SORT,
    A_SEARCHF,
    A_MAKEBAKS,
    A_TOTRAY,
    A_AUTOSAVE,
    A_FULLSCREEN,
    A_ZEN_MODE,
    A_SCALED,
    A_SCOLS,
    A_SROWS,
    A_SHOME,
    A_SEND,
    A_BORD0,
    A_BORD1,
    A_BORD2,
    A_BORD3,
    A_BORD4,
    A_BORD5,
    A_HSWAP,
    A_TEXTGRID,
    A_TAGADD,
    A_TAGREMOVE,
    A_WRAP,
    A_HIFY,
    A_FLATTEN,
    A_BROWSEF,
    A_ROUND0,
    A_ROUND1,
    A_ROUND2,
    A_ROUND3,
    A_ROUND4,
    A_ROUND5,
    A_ROUND6,
    A_FILTER5,
    A_FILTER10,
    A_FILTER20,
    A_FILTER50,
    A_FILTERM,
    A_FILTERL,
    A_FILTERS,
    A_FILTEROFF,
    A_FASTRENDER,
    A_EXPCSV,
    A_PASTESTYLE,
    A_PREVFILE,
    A_NEXTFILE,
    A_IMAGER,
    A_INCWIDTHNH,
    A_DECWIDTHNH,
    A_ZOOMSCR,
    A_ICONSET,
    A_V_GS,
    A_V_BS,
    A_V_LS,
    A_H_GS,
    A_H_BS,
    A_H_LS,
    A_GS,
    A_BS,
    A_LS,
    A_RESETSIZE,
    A_RESETWIDTH,
    A_RESETSTYLE,
    A_RESETCOLOR,
    A_LASTCELLCOLOR,
    A_LASTTEXTCOLOR,
    A_LASTBORDCOLOR,
    A_OPENCELLCOLOR,
    A_OPENTEXTCOLOR,
    A_OPENBORDCOLOR,
    A_DDIMAGE,
    A_MINCLOSE,
    A_SINGLETRAY,
    A_CENTERED,
    A_SORTD,
    A_STRIKET,
    A_FOLD,
    A_FOLDALL,
    A_UNFOLDALL,
    A_IMAGESCP,
    A_IMAGESCF,
    A_IMAGESCN,
    A_HELPI,
    A_HELP_OP_REF,
    A_REDO,
    A_FSWATCH,
    A_DEFBGCOL,
    A_THINSELC,
    A_COPYCT,
    A_MINISIZE,
    A_CUSTKEY,
    A_AUTOEXPORT,
    A_NOP,
//    A_TAGSET = 1000,  // and all values from here on
    A_SCRIPT = 2000,  // and all values from here on
//    A_MAXACTION = 3000
};

struct CfgCache {
    int roundness;
    int customcolor;
    bool singletray;
    bool minclose;
    bool totray;
    bool zoomscroll;
    bool thinselc;
    bool makebaks;
    bool autosave;
    bool fswatch;
    bool autohtmlexport;
    bool centered;
    bool fastrender;

};


class Config : public CfgCache
{
public:
    Config(bool portable);
    ~Config();

    QVariant read(const QString &key, const QVariant &def=QVariant()) const;

    void reset();
private:
    QSettings*setting;
};




// 全局配置
namespace _g {
// 默认调色板
extern uint celltextcolors[];
extern const int celltextcolors_size;
#define CUSTOMCOLORIDX 0

}

#endif // CONFIG_H
