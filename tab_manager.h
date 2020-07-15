#pragma once
#include <QTabWidget>
#include <QWebEngineHistory>
#include <QWebEngineFullScreenRequest>
#include "webview.h"
#include "setting_tab.h"
#include "debug_tab.h"
#include "fullscreen.h"
#include "request_filter.h"
#include "tab_groups.h"
#include "tox_ui.h"

struct TabWidget : public QTabWidget
{
    Q_OBJECT

public:

    TabWidget (QWidget *parent = nullptr);
    QWebEngineProfile *profile;
    QStandardItemModel model;

    SettingsTab *settings_tab();
    DebugTab *debug_tab();
    DebugTab *debug_tab_present;

signals:

    //void title_changed (const QString &title);
    void url_changed (const QUrl &url);
    void print_to_debug_tab (const QString &text);
    void reload_filters();

    void update_session();

public slots:
    void set_url (const QUrl &url, bool background=false);
    void close_page (int index);
    void close_tab (int index);
    WebView* create_tab();
    void restore_tab();
    void back();
    void forward();
    void refresh();
    void refresh_no_cache();
    void back_in_new_tab();
    void forward_in_new_tab();

    void save_state();
    void load_state();
    void download (QWebEngineDownloadItem* download);
    void cleanup();

private slots:
    void current_changed();
    void fullscreen_request (QWebEngineFullScreenRequest request);

private:
    RequestFilter* request_filter;
    QScopedPointer<FullScreenWindow> fullscreen;

    TabGroups tab_groups;
    TabGroup* assign_tab_group (QString host);

    QHash <QString, WebView*> host_views;

    void wheelEvent (QWheelEvent *event);

};
