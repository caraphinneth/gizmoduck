#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QWebEngineContextMenuData>

#include "browser_mainwindow.h"
#include "tab_manager.h"
#include "webview.h"

bool WebView::eventFilter (QObject* object, QEvent* event)
{
    if (object->parent() == this && event->type() == QEvent::MouseMove)
    {
        mouseMoveEvent (static_cast<QMouseEvent*>(event));
    }

    return false;
}

WebView::WebView (QWidget* parent): QWebEngineView (parent)
{
    QApplication::instance()->installEventFilter(this);
    setMouseTracking (true);

    connect (this, &WebView::selectionChanged, [this]()
    {
        // QToolTip::showText (mapToGlobal (position), selectedText());
    });
}

QWebEngineView* WebView::createWindow (QWebEnginePage::WebWindowType type)
{
    MainWindow* win = qobject_cast<MainWindow*>(window());
    if (!win)
        return nullptr;

/*
    switch (type)
    {
        case QWebEnginePage::WebBrowserTab:
            return win->tabWidget()->create_tab (false, false);

        case QWebEnginePage::WebBrowserBackgroundTab:
            return win->tabWidget()->create_tab (true, false);

        case QWebEnginePage::WebBrowserWindow:
            return win->tabWidget()->create_tab (false, false);

        case QWebEnginePage::WebDialog:
        {
            return win->tabWidget()->create_tab(true, false);
            //WebPopupWindow *popup = new WebPopupWindow(page()->profile());
            //return popup->view();
        }
    }

    return nullptr;
    */

    WebView* view = new WebView();
    connect (view, &WebView::urlChanged, this, &WebView::intercept_popup);
    return view;
}

void WebView::intercept_popup (const QUrl& url)
{
    if (WebView* view = qobject_cast<WebView*>(sender()))
    {
        emit link_requested (url.toString(), true);
        disconnect (view, &WebView::urlChanged, this, nullptr);
        view->deleteLater();
    }
}
void WebView::search_selected()
{
    QAction* action = qobject_cast<QAction*> (sender());

    if (!action)
      return;

    emit search_requested (action->data().toString());
}

void WebView::follow_link()
{
    QAction *action = qobject_cast<QAction*> (sender());

    if (!action)
      return;

    emit link_requested (action->data().toString(), false);
}

void WebView::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu* menu = page()->createStandardContextMenu();

    if (!menu)
        return;
    QWebEngineContextMenuData data_layer = page()->contextMenuData();

    if (!menu->actions().isEmpty())
    {

    }

    const QList<QAction*> actions = menu->actions();

    // Look for the default link option, and modify some.
    auto it = std::find (actions.cbegin(), actions.cend(), page()->action (QWebEnginePage::OpenLinkInThisWindow));
    if (it != actions.cend())
    {
        (*it)->setText(tr("Open Link in This Tab"));
        ++it;
        QAction *before (it == actions.cend() ? nullptr : *it);
        menu->insertAction(before, page()->action (QWebEnginePage::OpenLinkInNewWindow));
        menu->insertAction(before, page()->action (QWebEnginePage::OpenLinkInNewTab));
    }


    if (data_layer.isValid())
    {
        if (!data_layer.selectedText().isEmpty())
        {
            QFontMetrics fontMetric (menu->font());
            const QString elidedText = fontMetric.elidedText (data_layer.selectedText(), Qt::ElideRight, 100);
            QAction* action = new QAction (tr("Search \"")+elidedText+tr("\" on the web"), this);
            action->setData (data_layer.selectedText());
            connect (action, &QAction::triggered, this, &WebView::search_selected);
            menu->addAction (action);

            QUrl url = QUrl::fromUserInput (data_layer.selectedText());
            if (url.isValid())
            {
                QAction* action2 = new QAction (tr("Follow \"")+elidedText+"\"", this);
                action2->setData (data_layer.selectedText());
                connect (action2, &QAction::triggered, this, &WebView::follow_link);
                menu->addAction (action2);
                // action2->deleteLater ();
            }
            // action->deleteLater ();
        }
    }

    QAction* action3 = new QAction (tr("Translate Page"), this);
    action3->setData ("https://translate.google.com/translate?js=n&sl=auto&tl=en&u="+this->url().toString());
    connect (action3, &QAction::triggered, this, &WebView::follow_link);
    menu->addAction (action3);

    menu->addAction (page()->action (QWebEnginePage::InspectElement));

    menu->popup (event->globalPos());
}

void WebView::mouseMoveEvent (QMouseEvent* event)
{
    position = event->pos();
    // qDebug() << event->x() << event->y();
    QWebEngineView::mouseMoveEvent (event);
}
/*
QSize WebView::sizeHint (void) const
{
    //return QApplication::desktop()->screenGeometry (this).size();
    MainWindow *win = qobject_cast<MainWindow*>(window());
    return QSize (win->tabWidget()->size().width()-win->tabWidget()->tabBar()->width(), win->tabWidget()->size().height());
}

QSize WebView::minimumSizeHint (void) const
{
    MainWindow *win = qobject_cast<MainWindow*>(window());
    return QSize (win->tabWidget()->size().width()-win->tabWidget()->tabBar()->width()-64, win->tabWidget()->size().height()-64);
}*/