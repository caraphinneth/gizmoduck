#pragma once
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QWebEngineHistory>
#include <QWebEngineFullScreenRequest>

#include "debug_tab.h"
#include "fullscreen.h"
#include "process_manager.h"
#include "process_tab.h"
#include "request_filter.h"
#include "setting_tab.h"
#include "side_tabs.h"
#include "tab_groups.h"
#include "tox_ui.h"
#include "webview.h"

struct TabWidget: public QStackedWidget
{
    Q_OBJECT

public:
    TabWidget (QWidget* parent = nullptr);

    SideTabs* tabBar;

    QWebEngineProfile* profile;
    QStandardItemModel suggestions;

    History history;

    SettingsTab* settings_tab();
    DebugTab* debug_tab();
    ProcessTab* show_process_tab();

signals:
    //void title_changed(const QString &title);
    void url_changed(const QUrl& url);
    void url_change_confirmed(const QUrl& url);
    void print_to_debug_tab(const QString& text);
    void reload_filters();
    void avatar_changed();
    void name_update(const QString& name);
    void status_update(const QString& status);
    void update_session();
    void debug_tabs_updated();
    void page_requested(QWeakPointer<WebPage> page);

public slots:
    void set_url(const QUrl& url, bool background=false);
    void close_page(int index);
    void close_tab(int index);
    WebView* create_tab(bool at_end=false);
    void restore_tab();
    void back();
    void forward();
    void refresh();
    void refresh_no_cache();
    void back_in_new_tab();
    void forward_in_new_tab();

    void save_state();
    void load_state();
    void download(QWebEngineDownloadRequest* download);
    void cleanup();

private slots:
    void current_changed (int index);
    void fullscreen_request (QWebEngineFullScreenRequest request);

private:
    QTimer* autosave;
    ProcessManager* process_manager;
    ProcessTab* process_tab;
    RequestFilter* request_filter;
    QScopedPointer<FullScreenWindow> fullscreen;
    void manage_page (const QUrl& url_old, const QUrl& url_new);

    TabGroups tab_groups;
    TabGroup* assign_tab_group (const QString& host);
    QHash <QString, WebView*> host_views;
    WebView *assign_host_view(const QString& host);

    WebView* current_view();

    //void wheelEvent (QWheelEvent* event);
    QWeakPointer<WebPage> page_back (TabGroup* group);
    QWeakPointer<WebPage> page_forward (TabGroup* group);

    void setTabIcon(int index, const QIcon& icon);
    void setTabText(int index, const QString& text);
    void set_page(WebView* view, const QWeakPointer<WebPage> page);

    void page_signal_handler(QSharedPointer<WebPage> p, const QUrl& new_url);
};
