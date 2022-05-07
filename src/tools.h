#ifndef TOOLS_H
#define TOOLS_H

#include <QString>

namespace Tools {

//! 解析路径到真实路径
QString resolvePath(const QString &path, bool exist);

}

#define DELPTR(p) ({if (p) {delete p; p = nullptr;}})

#endif // TOOLS_H
