#include <QContextMenuEvent>
#include <QMenu>
#include <QWebEngineContextMenuData>
#include <QMouseEvent>
#include "browser_mainwindow.h"
#include "tab_manager.h"
#include "webview.h"
#include <QApplication>
#include "QDebug"
//#include <QTimer>
//#include <QStackedLayout>
//#include <QtGlobal>

/*#if QT_VERSION < 0x051000
bool WebView::eventFilter (QObject *object, QEvent *event)
{
    // It does not really fly. It's just so ugly Mother Earth rejects it.

    if ((event->type() == QEvent::Wheel)&&(object==proxy))
    {
        bool wasAccepted = event->isAccepted();
        event->setAccepted(false);

        if (event->spontaneous())
        {
            QWheelEvent *mouse_event=static_cast<QWheelEvent*>(event);

            QWheelEvent e (mouse_event->pos(), mouse_event->globalPos(), mouse_event->pixelDelta(), mouse_event->angleDelta() * QApplication::wheelScrollLines() / 10.0, 0,
                           Qt::Horizontal, mouse_event->buttons(), mouse_event->modifiers(), mouse_event->phase(), mouse_event->source(), mouse_event->inverted());
            QApplication::sendEvent (object, &e);
            mouse_event->accept();
        }
        bool ret = event->isAccepted();
        event->setAccepted (wasAccepted);
        return ret;
    }
    //if ((event->type() == QEvent::Wheel)&&(object==this)) return true;
    //const bool res = QWebEngineView::eventFilter(object, event);
    return false;
}
#endif*/

bool WebView::eventFilter(QObject *object, QEvent *event)
{
    if (object->parent() == this && event->type() == QEvent::MouseMove)
    {
        mouseMoveEvent (static_cast<QMouseEvent *>(event));
    }

    return false;
}

WebView::WebView (QWidget *parent): QWebEngineView (parent)
{
    QApplication::instance()->installEventFilter(this);
    setMouseTracking (true);

    connect (this, &WebView::selectionChanged, [this]()
    {
        // QToolTip::showText (mapToGlobal (position), selectedText());
    });

/*
#if QT_VERSION < 0x051000

    installEventFilter (this);
    if (parentWidget()) {
        parentWidget()->installEventFilter(this);
    }
    QStackedLayout *l = qobject_cast<QStackedLayout*>(layout());
        connect(l, &QStackedLayout::currentChanged, this, [this]() {

    QTimer::singleShot (0, this, [this]()
    {
        proxy = focusProxy();
        proxy->installEventFilter (this);
    });
    });
#endif
*/
}

QWebEngineView *WebView::createWindow (QWebEnginePage::WebWindowType type)
{
    MainWindow *win = qobject_cast<MainWindow*>(window());
    if (!win)
        return nullptr;

    switch (type)
    {
        case QWebEnginePage::WebBrowserTab:
            return win->tabWidget()->create_tab (false);

        case QWebEnginePage::WebBrowserBackgroundTab:
            return win->tabWidget()->create_tab (true);

        case QWebEnginePage::WebBrowserWindow:
            return win->tabWidget()->create_tab (false);

        case QWebEnginePage::WebDialog:
        {
            return win->tabWidget()->create_tab();
            //WebPopupWindow *popup = new WebPopupWindow(page()->profile());
            //return popup->view();
        }
    }
    return nullptr;
}

void WebView::search_selected ()
{
    QAction *action = qobject_cast<QAction *> (sender());

    if (!action)
      return;

    emit search_requested (action->data().toString());
}

void WebView::follow_link ()
{
    QAction *action = qobject_cast<QAction *> (sender());

    if (!action)
      return;

    emit link_requested (action->data().toString());
}

void WebView::contextMenuEvent (QContextMenuEvent *event)
{
    QMenu *menu = page()->createStandardContextMenu();

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
            QAction *action = new QAction (tr("Search \"")+elidedText+tr("\" on the web"), this);
            action->setData (data_layer.selectedText());
            connect (action, &QAction::triggered, this, &WebView::search_selected);
            menu->addAction (action);

            QUrl url = QUrl::fromUserInput (data_layer.selectedText());
            if (url.isValid())
            {
                QAction *action2 = new QAction (tr("Follow \"")+elidedText+"\"", this);
                action2->setData (data_layer.selectedText());
                connect (action2, &QAction::triggered, this, &WebView::follow_link);
                menu->addAction (action2);
                // action2->deleteLater ();
            }
            // action->deleteLater ();
        }
    }

    QAction *action3 = new QAction (tr("Translate Page"), this);
    action3->setData ("http://translate.google.com/translate?js=n&sl=auto&tl=en&u="+this->url().toString());
    connect (action3, &QAction::triggered, this, &WebView::follow_link);
    menu->addAction (action3);

    menu->addAction (page()->action (QWebEnginePage::InspectElement));

    menu->popup (event->globalPos());
}

void WebView::mouseMoveEvent (QMouseEvent *event)
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