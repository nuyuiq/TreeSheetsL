#include "config.h"
#include "symdef.h"
#include "tools.h"

#include <QSettings>
#include <QDebug>

Config::Config(bool portable)
{
    if (portable)
    {
        setting = new QSettings(Tools::resolvePath(QString(), true) + "/" CFG_FILEPATH, QSettings::IniFormat);
    }
    else
    {
        setting = new QSettings(QSettings::IniFormat, QSettings::UserScope, "TreeSheets", "TreeSheets");
    }
    qDebug() << "Cfg filename: " << setting->fileName();

    reset();
}

Config::~Config()
{
    DELPTR(setting);
}

QVariant Config::read(const QString &key, const QVariant &def) const
{
    return setting->value(key, def);
}

void Config::write(const QString &key, const QVariant &value)
{
    setting->setValue(key, value);
}

void Config::reset()
{
    _g::deftextsize = read(QStringLiteral("defaultfontsize"), _g::deftextsize).toInt();
#ifdef Q_OS_WIN
    const QString &df = QStringLiteral("Lucida Sans Unicode");
#else
    const QString &df = QStringLiteral("Verdana");
#endif
    defaultfont = read(QStringLiteral("defaultfont"), df).toString();
    defaultmaxcolwidth = COLWIDTH_DEFAULTMAX;
    customcolor = 0xffffff;
    singletray = read(QStringLiteral("singletray"), false).toBool();
    roundness = read(QStringLiteral("roundness"), 3).toInt();
    minclose = read(QStringLiteral("minclose"), false).toBool();
    totray = read(QStringLiteral("totray"), false).toBool();
    zoomscroll = read(QStringLiteral("zoomscroll"), false).toBool();
    thinselc = read(QStringLiteral("thinselc"), true).toBool();
    makebaks = read(QStringLiteral("makebaks"), true).toBool();
    autosave = read(QStringLiteral("autosave"), true).toBool();
    fswatch = read(QStringLiteral("fswatch"), true).toBool();
    autohtmlexport = read(QStringLiteral("autohtmlexport"), false).toBool();
    centered = read(QStringLiteral("centered"), true).toBool();
    fastrender = read(QStringLiteral("fastrender"), true).toBool();

    pen_tinytext = 0x808080;
    pen_tinygridlines = 0xf2dcd8;
    pen_gridlines = 0xe5b7b0;
    pen_thinselect = 0xc0c0c0;
}


uint _g::celltextcolors[] = {
    0xFFFFFF,  // CUSTOM COLOR!
    0xFFFFFF, 0x000000, 0x202020, 0x404040, 0x606060, 0x808080, 0xA0A0A0, 0xC0C0C0, 0xD0D0D0,
    0xE0E0E0, 0xE8E8E8, 0x000080, 0x0000FF, 0x8080FF, 0xC0C0FF, 0xC0C0E0, 0x008000, 0x00FF00,
    0x80FF80, 0xC0FFC0, 0xC0E0C0, 0x800000, 0xFF0000, 0xFF8080, 0xFFC0C0, 0xE0C0C0, 0x800080,
    0xFF00FF, 0xFF80FF, 0xFFC0FF, 0xE0C0E0, 0x008080, 0x00FFFF, 0x80FFFF, 0xC0FFFF, 0xC0E0E0,
    0x808000, 0xFFFF00, 0xFFFF80, 0xFFFFC0, 0xE0E0C0,
};

const int _g::celltextcolors_size = sizeof (_g::celltextcolors) / sizeof (_g::celltextcolors[0]);
const int _g::grid_margin = 1;
const int _g::cell_margin = 2;
const int _g::margin_extra = 2;  // TODO, could make this configurable: 0/2/4/6
const int _g::line_width = 1;
const int _g::selmargin = 2;
int _g::deftextsize = 12;
int _g::mintextsize() { return _g::deftextsize - 8; }
int _g::maxtextsize() { return _g::deftextsize + 32; }
const int _g::grid_left_offset = 15;
const int _g::scrollratecursor = 240;  // FIXME: must be configurable
const int _g::scrollratewheel = 2;     // relative to 1 step on a fixed wheel usually being 120
