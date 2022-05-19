#ifndef SYMDEF
#define SYMDEF

// 语言文件目录
#define TR_FILEPATH "translations"

// 图片资源
#define IMG_FILEPATH "images"

// 服务器绑定端口/UDP
#define PORT_SERVER 4242

// 配置文件
#define CFG_FILEPATH "TreeSheets.ini"

// 打开历史最大条目
#define MAXCOUNT_HISTORY 9

// 图标
#define ICON_FILEPATH "/icon16.png"

// 工具条/状态栏背景色    // RGB
#define TOOL_BGCOLOR0 "#E8ECF0"
#define TOOL_BGCOLOR1 "#BCC7D8"

// 工具条原始尺寸
#define TOOL_SIZE0 18
#define TOOL_SIZE1 22

// 工具栏图标
#define TOOL_ICON0 "images/webalys/toolbar"
#define TOOL_ICON1 "images/nuvola/toolbar"

// 默认最大列宽
#define COLWIDTH_DEFAULTMAX 80

// 滚轮的步长增量
#define WHEELDELTA 120

// 小图标默认缩放
#define dd_icon_res_scale 3.0

#ifdef Q_OS_WIN
#define PATH_SEPERATOR "\\"
#define LINE_SEPERATOR "\r\n"
#else
#define PATH_SEPERATOR "/"
#ifdef Q_OS_MAC
#define LINE_SEPERATOR "\r"
#else
#define LINE_SEPERATOR "\n"
#endif
#endif

#endif // SYMDEF

