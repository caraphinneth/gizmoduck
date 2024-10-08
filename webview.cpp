#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QWebEngineContextMenuRequest>
#include <QProcess>
#include <QSettings>

#include "browser_mainwindow.h"
#include "tab_manager.h"
#include "webview.h"

/*
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

}
*/

WebView::WebView(QWebEngineProfile* profile, QWidget* parent): QWebEngineView (profile, parent)
{
    /*
    QApplication::instance()->installEventFilter(this);
    setMouseTracking (true);

    connect (this, &WebView::selectionChanged, [this]()
    {
        // QToolTip::showText (mapToGlobal (position), selectedText());
    });
    */
}

QWebEngineView* WebView::createWindow (QWebEnginePage::WebWindowType type)
{
    MainWindow* win = qobject_cast<MainWindow*>(window());
    if (!win)
        return nullptr;

    bool background = (type == QWebEnginePage::WebBrowserBackgroundTab);

    // Have to do this ugly profile passthrough because Qt6.
    WebView* view = new WebView(win->tab_manager->profile, win->tab_manager);
    // connect (view, &WebView::urlChanged, this, &WebView::intercept_popup);
    connect (view, &WebView::urlChanged, [this, view, background](const QUrl& url)
    {
        // TODO: redirectors
        if (url.host()!="t.co")
        {
            emit link_requested(url.toString(), background);
            view->disconnect();
            view->deleteLater();
        }
    });
    return view;
}

void WebView::search_selected()
{
    QAction* action = qobject_cast<QAction*> (sender());

    if (!action)
      return;

    emit search_requested(action->data().toString());
}

void WebView::follow_link()
{
    QAction *action = qobject_cast<QAction*> (sender());

    if (!action)
      return;

    emit link_requested(action->data().toString(), false);
}

void WebView::run_yt_dlp()
{
    QAction* action = qobject_cast<QAction*> (sender());

    if (!action)
      return;

    QSettings settings;
    settings.beginGroup("External");
    QString default_args = settings.value("commandline", "").toString();
    settings.endGroup();

    QString cookie_path = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("QtWebEngine/Default/Cookies");
    QString download_path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QStringList arg_list = default_args.split(' ', Qt::SkipEmptyParts);

    arg_list << "-C" << cookie_path << "--windows-filenames" << "-P" << download_path << action->data().toString();

    // qDebug() << "Argument list:" << arg_list;

    emit process_requested("yt-dlp", arg_list);
}

void WebView::run_gallery_dl()
{
    QAction* action = qobject_cast<QAction*> (sender());

    if (!action)
      return;

    QSettings settings;
    settings.beginGroup("External");
    QString default_args = settings.value("commandline", "").toString();
    settings.endGroup();

    QString cookie_path = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("QtWebEngine/Default/Cookies");
    QString download_path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QStringList arg_list = default_args.split(' ', Qt::SkipEmptyParts);

    arg_list << "-C" << cookie_path << "-d" << download_path << action->data().toString();

    // qDebug() << "Argument list:" << arg_list;

    emit process_requested("gallery-dl", arg_list);
}

void WebView::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu* menu = createStandardContextMenu();

    if (!menu)
        return;

    if (!menu->actions().isEmpty())
    {

    }

    QWebEngineContextMenuRequest* data = lastContextMenuRequest();

    const QList<QAction*> actions = menu->actions();

    // Look for the default link option, and modify some.
    auto it = std::find(actions.cbegin(), actions.cend(), page()->action(QWebEnginePage::OpenLinkInThisWindow));
    if (it != actions.cend())
    {
        (*it)->setText(tr("Open Link in This Tab"));
        ++it;
        QAction* before (it == actions.cend() ? nullptr : *it);
        menu->insertAction(before, page()->action (QWebEnginePage::OpenLinkInNewWindow));
        menu->insertAction(before, page()->action (QWebEnginePage::OpenLinkInNewTab));
    }


    {
        if (!data->selectedText().isEmpty())
        {
            QFontMetrics fontMetric (menu->font());
            const QString elidedText = fontMetric.elidedText(data->selectedText(), Qt::ElideRight, 100);
            QAction* action = new QAction (tr("Search \"")+elidedText+tr("\" on the web"), this);
            action->setData(data->selectedText());
            connect(action, &QAction::triggered, this, &WebView::search_selected);
            menu->addAction (action);

            QUrl url = QUrl::fromUserInput (data->selectedText());
            if (url.isValid())
            {
                QAction* action2 = new QAction (tr("Follow \"")+elidedText+"\"", this);
                action2->setData(data->selectedText());
                connect (action2, &QAction::triggered, this, &WebView::follow_link);
                menu->addAction (action2);
                // action2->deleteLater ();
            }
            // action->deleteLater ();
        }
    }

    QAction* action3 = new QAction(tr("Translate Page"), this);
    action3->setData ("https://translate.google.com/translate?js=n&sl=auto&tl=en&u="+this->url().toString());
    connect (action3, &QAction::triggered, this, &WebView::follow_link);
    menu->addAction(action3);

    QAction* action4 = new QAction (tr("Run yt-dlp"), this);
    action4->setData(this->url().toString());
    connect (action4, &QAction::triggered, this, &WebView::run_yt_dlp);
    menu->addAction(action4);

    QAction* action5 = new QAction (tr("Run gallery-dl"), this);
    action5->setData(this->url().toString());
    connect (action5, &QAction::triggered, this, &WebView::run_gallery_dl);
    menu->addAction(action5);

    menu->addAction (page()->action(QWebEnginePage::InspectElement));

    menu->popup (event->globalPos());
}

/*
void WebView::mouseMoveEvent (QMouseEvent* event)
{
    position = event->pos();
    // qDebug() << event->x() << event->y();
    QWebEngineView::mouseMoveEvent (event);
}
*/
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
