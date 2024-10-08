QT += widgets webenginewidgets sql
CONFIG += c++20
CONFIG -= app_bundle
DEFINES += QT_MESSAGELOGCONTEXT
DEFINES += QT_SHAREDPOINTER_TRACK_POINTERS
LIBS += -ljemalloc

win32:QMAKE_CXXFLAGS+="-O2"
!win32 {
    QMAKE_CXXFLAGS+="-O3 -pipe -march=native -ftree-vectorize -fno-predictive-commoning -fno-semantic-interposition -floop-nest-optimize -fipa-pta -fdevirtualize-at-ltrans -Wall -Wextra -Wpedantic -flto=6"
    #QMAKE_CXXFLAGS+="-Og -pipe -march=native -Wall -Wextra -Wpedantic"
}

#-g -ggdb -pg -ggdb3"
#-flto=8"
#QMAKE_LFLAGS+="-pg"

!win32 {
    LIBS += -ltoxcore -lsodium
}
#Update those paths and supply pthreadVC2/libsodium/toxcore dlls if linking dynamically.
win32 {
    LIBS += c:/coding/pthreadVC2.lib
    LIBS += c:/coding/libsodium.lib
    LIBS += c:/coding/toxcore.lib
    LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.18362.0/um/x64/iphlpapi.Lib"
    LIBS += "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.18362.0/um/x64/ws2_32.Lib"
}
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
    fullscreen.cpp \
    input_widget.cpp \
    message_log.cpp \
    process_manager.cpp \
    process_tab.cpp \
    tab_manager.cpp \
    webview.cpp \
    navigation_button.cpp \
    webpage.cpp \
    file_dialog.cpp \
    side_tabs.cpp \
    settings_tab.cpp \
    debug_tab.cpp \
    userscript.cpp \
    request_filter.cpp \
    tox_client.c \
    tox_ui.cpp

win32 {
HEADERS += \
    asprintf.h \
}

HEADERS += \
    browser_mainwindow.h \
    fullscreen.h \
    input_widget.h \
    message_log.h \
    process_manager.h \
    process_tab.h \
    tab_groups.h \
    tab_manager.h \
    webview.h \
    navigation_button.h \
    webpage.h \
    file_dialog.h \
    request_filter.h \
    side_tabs.h \
    setting_tab.h \
    debug_tab.h \
    userscript.h \
    tox_client.h \
    tox_ui.h

TRANSLATIONS = gizmoduck_ru.ts

RESOURCES += \
    browser.qrc
