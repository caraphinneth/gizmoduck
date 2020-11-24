#pragma once
#include <QWebEngineView>
#include "tab_groups.h"
#include "webpage.h"

struct WebView: public QWebEngineView
{
    Q_OBJECT

public:
    WebView (QWidget* parent = nullptr);

protected:
    void contextMenuEvent (QContextMenuEvent* event) override; // Our own context menu.
    void mouseMoveEvent (QMouseEvent* event) override;
    QWebEngineView* createWindow (QWebEnginePage::WebWindowType type) override; // Overload to handle links requiring new window.
    bool eventFilter (QObject* object, QEvent* event);

signals:
    void icon_changed (const QIcon& icon);
    void search_requested (const QString& text);
    void link_requested (const QString& url, bool background);

private:
    void search_selected();
    void follow_link();
    void intercept_popup(const QUrl& url);

    QPoint position;
};
