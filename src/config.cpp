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

void Config::reset()
{
    singletray = read(QStringLiteral("singletray"), false).toBool();
}
