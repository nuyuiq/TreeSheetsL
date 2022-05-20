QT       += core gui network xml printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    history.cpp \
    main.cpp \
    mainwindow.cpp \
    myapp.cpp \
    widget.cpp \
    tools.cpp \
    mylog.cpp \
    server.cpp \
    config.cpp \
    dropdown.cpp \
    document.cpp \
    cell.cpp \
    grid.cpp \
    text.cpp \
    selection.cpp \
    action.cpp \
    image.cpp

HEADERS += \
    history.h \
    mainwindow.h \
    myapp.h \
    widget.h \
    symdef.h \
    tools.h \
    mylog.h \
    server.h \
    config.h \
    dropdown.h \
    document.h \
    image.h \
    cell.h \
    grid.h \
    text.h \
    selection.h

FORMS +=

DESTDIR = $$PWD/../app


# DEFINES += SIMPLERENDER


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

