#pragma once
#include <QWebEngineView>
#include "tab_groups.h"
#include "webpage.h"

struct WebView: public QWebEngineView
{
    Q_OBJECT

public:
    WebView (QWidget* parent=nullptr);
    WebView (QWebEngineProfile* profile, QWidget* parent=nullptr);

protected:
    void contextMenuEvent (QContextMenuEvent* event) override; // Our own context menu.
    QWebEngineView* createWindow (QWebEnginePage::WebWindowType type) override; // Overload to handle links requiring new window.

    // Experimental
    // void mouseMoveEvent (QMouseEvent* event) override;
    // bool eventFilter (QObject* object, QEvent* event);

signals:
    void icon_changed (const QIcon& icon);
    void search_requested (const QString& text);
    void link_requested (const QString& url, bool background);
    void process_requested(const QString& executable, const QStringList& arguments);
    void went_to_sleep();

private:
    void search_selected();
    void follow_link();
    void run_yt_dlp();

public slots:
    void set_page(QWeakPointer<WebPage> page);

    // QPoint position;
};
