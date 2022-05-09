#ifndef TOOLS_H
#define TOOLS_H

#include <QString>

namespace Tools {

//! 解析路径到真实路径
QString resolvePath(const QString &path, bool exist);

QString bakName(const QString &filename);
QString tmpName(const QString &filename);
QString extName(const QString &filename, const QString &ext);

}

#define DELPTR(p) ({if (p) {delete p; p = nullptr;}})

#endif // TOOLS_H
