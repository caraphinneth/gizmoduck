#pragma once
#include <QTabWidget>
#include <QWebEngineHistory>
#include "webview.h"
#include "setting_tab.h"
#include "debug_tab.h"
#include "request_filter.h"
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

    void set_url (const QUrl &url);
    void close_tab (int index);
    WebView *create_tab (bool background=false);
    void restore_tab();
    void back();
    void forward();
    void refresh();
    void refresh_no_cache();
    void open_in_background_tab (const QUrl &url);
    void back_in_new_tab();
    void forward_in_new_tab();
    void suspend (int i);
    void resume (int i);
    void save_state();
    void load_state();
    void download (QWebEngineDownloadItem *download);


private slots:

    void current_changed();

private:
    RequestFilter* request_filter;

};
