#pragma once
#include <QWebEngineView>
//#if QT_VERSION < 0x051000
//#include <QPointer>
//#endif

struct WebView : public QWebEngineView
{
    Q_OBJECT

public:
    WebView (QWidget *parent = nullptr);

    // int loadProgress() const;
   // bool isWebActionEnabled(QWebEnginePage::WebAction webAction) const;
//    QIcon favIcon() const;

protected:
    void contextMenuEvent (QContextMenuEvent *event) override; // Our own context menu.
    void mouseMoveEvent (QMouseEvent *event) override;
    QWebEngineView *createWindow (QWebEnginePage::WebWindowType type) override; // Overload to handle links requiring new window.
    bool eventFilter (QObject *object, QEvent *event);

    //QSize sizeHint (void) const;
    //QSize minimumSizeHint (void) const;

signals:
   // void webActionEnabledChanged(QWebEnginePage::WebAction webAction, bool enabled);
    void icon_changed (const QIcon &icon);
    void search_requested (QString text);
    void link_requested (QString url);

private:
    void search_selected();
    void follow_link();

    QPoint position;
   // void createWebActionTrigger(QWebEnginePage *page, QWebEnginePage::WebAction);
/*#if QT_VERSION < 0x051000

   // int m_loadProgress;
    QPointer<QWidget> proxy;
endif*/
};
