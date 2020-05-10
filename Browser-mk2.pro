QT += widgets webenginewidgets sql

CONFIG += c++14 exceptions_off
CONFIG -= app_bundle

QMAKE_CXXFLAGS+="-O2 -march=native -ftree-vectorize -floop-interchange -ftree-loop-distribution -floop-strip-mine -floop-block"
#QMAKE_LFLAGS+="-flto"

LIBS += -ltoxcore -lsodium

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    browser_mainwindow.cpp \
    dockwidget.cpp \
    fullscreen.cpp \
    message_log.cpp \
    tab_manager.cpp \
    tox_client.c \
    tox_ui.cpp \
    webview.cpp \
    navigation_button.cpp \
    webpage.cpp \
    file_dialog.cpp \
    side_tabs.cpp \
    settings_tab.cpp \
    gl_widget.cpp \
    debug_tab.cpp \
    userscript.cpp \
    request_filter.cpp

HEADERS += \
    browser_mainwindow.h \
    dockwidget.h \
    fullscreen.h \
    message_log.h \
    tab_manager.h \
    tox_client.h \
    tox_ui.h \
    webview.h \
    navigation_button.h \
    webpage.h \
    file_dialog.h \
    request_filter.h \
    side_tabs.h \
    setting_tab.h \
    gl_widget.h \
    debug_tab.h \
    userscript.h

TRANSLATIONS = gizmoduck_ru.ts

RESOURCES += \
    browser.qrc
