#include <QToolButton>
#include <QHBoxLayout>
#include <QEvent>
#include <QSaveFile>
#include <QWebEngineScriptCollection>
#include "QTabBar"
#include "QWebEngineProfile"
#include "QWebEngineSettings"
#include "tab_manager.h"
#include "webpage.h"
#include "side_tabs.h"
#include "userscript.h"

// The tab widget with web views and more for tab contents. Navigation functions are also handled by this one.
TabWidget::TabWidget (QWidget *parent): QTabWidget (parent)
{
    // Create QWebEngineProfile with the settings we prefer.
    profile =  QWebEngineProfile::defaultProfile();
    //profile->settings()->setAttribute (QWebEngineSettings::ErrorPageEnabled, false); // Disable Chromium error pages for now (since they look ugly when adblocking).
    profile->settings()->setAttribute (QWebEngineSettings::FullScreenSupportEnabled, true);
    //profile->settings()->setAttribute (QWebEngineSettings::ScreenCaptureEnabled,true);
    profile->settings()->setAttribute (QWebEngineSettings::JavascriptCanOpenWindows, false);
    profile->settings()->setAttribute (QWebEngineSettings::PluginsEnabled, true);
    profile->settings()->setAttribute (QWebEngineSettings::XSSAuditingEnabled, false);
    //profile->settings()->setAttribute (QWebEngineSettings::FocusOnNavigationEnabled, true);
    profile->settings()->setAttribute (QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
    profile->settings()->setAttribute (QWebEngineSettings::PlaybackRequiresUserGesture, false);
    profile->settings()->setAttribute (QWebEngineSettings::DnsPrefetchEnabled, true);
    //profile->settings()->setUnknownUrlSchemePolicy (QWebEngineSettings::AllowAllUnknownUrlSchemes);

    profile->settings()->setFontFamily (QWebEngineSettings::StandardFont, "Roboto");
    profile->settings()->setFontFamily (QWebEngineSettings::SerifFont, "Roboto");
    profile->settings()->setFontFamily (QWebEngineSettings::SansSerifFont, "Roboto");


    profile->setSpellCheckEnabled (false);

    // remove Chromium orange outlines, also style broken images a bit.
    UserScript custom_css;
    custom_css.load_from_file (":/scripts/custom_css");
    custom_css.setInjectionPoint (QWebEngineScript::DocumentReady);
    //custom_css.setRunsOnSubFrames (true);
    //custom_css.setWorldId(QWebEngineScript::ApplicationWorld);
    profile->scripts()->insert (custom_css);

    /* UserScript jade_script;
    jade_script.load_from_file ("userscripts/jade_script.js");
    jade_script.setInjectionPoint (QWebEngineScript::DocumentReady);
    jade_script.setName ("Jade script");
     profile->scripts()->insert (jade_script);*/

    RequestFilter *request_filter = new RequestFilter;
    profile->setUrlRequestInterceptor (request_filter);

    //profile->setHttpUserAgent ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Gizmoduck/0.1.1 Chrome/61.0.3163.140 Safari/537.36");
    //profile->setHttpUserAgent ("Mozilla/5.0 (X11; Linux x86_64; rv:71.0) Gecko/20100101 Firefox/71.0");
    //profile->setHttpAcceptLanguage ("ru, en-gb, en-us;q=0.9, en;q=0.8|ru");

    connect (profile, &QWebEngineProfile::downloadRequested, this, &TabWidget::download); // Should be moved to a separate widget later.

    SideTabs *tabBar = new SideTabs (this, 162, 40);
    setTabBar (tabBar);

    //tabBar->setTabsClosable (true);
    //tabBar->setSelectionBehaviorOnRemove (QTabBar::SelectPreviousTab);
    tabBar->setMovable (true);

    setTabPosition (East);
    tabBar->setContextMenuPolicy (Qt::CustomContextMenu);
    //setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Ignored);

    connect (tabBar, &QTabBar::tabCloseRequested, this, &TabWidget::close_tab);

    connect (tabBar, &QTabBar::tabBarDoubleClicked, [this]()
    {
        create_tab (false); // Lamdba does not honour default args?..
    });

    setDocumentMode (true);
    //setElideMode (Qt::ElideRight);

    connect(this, &QTabWidget::currentChanged, this, &TabWidget::current_changed);
    connect (this, &TabWidget::update_session, this, &TabWidget::save_state);
    connect (this, &TabWidget::reload_filters, request_filter, &RequestFilter::ReloadLists);

    load_state();

    if (!count())
        create_tab ();

    connect (request_filter, &RequestFilter::debug_message, this, &TabWidget::print_to_debug_tab);
}


// Create a new web tab, link its events to interface where needed.
WebView *TabWidget::create_tab (bool background)
{
    WebView *view = new WebView (this);
    WebPage *page = new WebPage (profile, view);

    view->setPage (page);
    view->setAttribute(Qt::WA_DontShowOnScreen);
    insertTab (currentIndex()+1, view, tr("Untitled"));

    /*
    if (background)
    {
        //view->setFixedSize (QSize (size().width()-tabBar()->width()-1, size().height()));
        //updateGeometry();
        view->resize (QSize (size().width()-tabBar()->width(), size().height())); //If you make the webview a child, resize may be needed.
        view->move (this->pos());
        view->show();
    }
    */
    // Close buttons on side tabs are hard to make good. Disabled for now.
    /*QWidget *widget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(this);
    widget->setLayout(layout);

    QToolButton *closeBtn = new QToolButton(this);
    layout->addWidget (closeBtn);
    layout->insertSpacing(-15, -120);
    closeBtn->setStyleSheet("max-height: 16px; max-width: 16px;");

    tabBar()->setTabButton(indexOf(view), QTabBar::RightSide, widget);
    closeBtn->hide();*/

    // Autoassign tab title.
    connect (view, &QWebEngineView::titleChanged, [this, view] (const QString &title)
    {
        int index = indexOf (view);
        // Work around the bug(?) with returning url sometimes on heavily updated titles (may cause flicker).
        if ((index != -1) && (!title.startsWith ("http")) && (title!="about:blank"))
        {
            setTabText (index, title);
            QList<QStandardItem *> vl (model.findItems (view->url().toString()));
            if (vl.isEmpty ())
            {
                if (!view->url().toString().contains (title))
                {
                    QStandardItem *item1 = new QStandardItem (view->url().toString());
                    QStandardItem *item2 = new QStandardItem (title);
                    QList <QStandardItem*> list;
                    list.append (item1);
                    list.append (item2);
                    model.insertRow (0, list);
                    if (model.rowCount()>5000)
                        model.setRowCount (5000);
                    list.clear();
                }
            }
            else
            {
                QList<QStandardItem *> list = model.takeRow (vl.at(0)->row());
                //list.append (model.takeRow (vl.at(0)->row()));
                model.insertRow (0, list);
                list.clear();
            }
            vl.clear();
        }
        //if (currentIndex() == index)
          //  emit title_changed (title);
    });

    // Better than linking at top level, since url may change with both navigation and tab switching.
    connect (view, &QWebEngineView::urlChanged, [this, view] (const QUrl &url)
    {
        int index = indexOf (view);
        if (index != -1)
        {
            tabBar()->setTabData (index, url);
            //emit update_session(); // This may be too often.
        }
        if (currentIndex() == index)
           emit url_changed (url);
    });

    // Set tab icon according to favicon.
    connect (view, &QWebEngineView::iconChanged, [this, view] (const QIcon &icon)
    {
        int index = indexOf (view);
        if (index != -1)
        {
            setTabIcon (index, icon);
            //print_to_debug_tab (view->iconUrl().toString());

        }
    });

    /*connect (view, &QWebEngineView::loadFinished, [this, view]
    {
        int index = indexOf (view);
        if ((index != -1) && (view->icon().isNull()))
        {
            setTabIcon (index, (QIcon (QStringLiteral (":/icons/gizmoduck"))));
        }
    });*/

    // Enforce the same if the page finished loading... since 'loading' icon is not so elegantly implemented atm.
    connect (view, &QWebEngineView::loadProgress, [this, view] (int progress)
    {
        int index = indexOf (view);

        if ((index != -1) && (progress==100))
        {
           if (view->icon().isNull())
               setTabIcon (index, (QIcon (QStringLiteral (":/icons/gizmoduck"))));
           else
               setTabIcon (index, view->icon());
           //print_to_debug_tab (view->iconUrl().toString());
           emit update_session();
        }
    });


    // Clear the icon on page load start, so 'loading' icon can take over.
    /*connect (view, &QWebEngineView::loadStarted, [this, view]
    {
        int index = indexOf (view);
        if ((index != -1)&&(!view->loadProgress()<100))
        {
            setTabIcon (index, QIcon());
        }
    });*/

    // Install a handler for the context menu action.
    connect (view, &WebView::search_requested, [this] (QString text)
    {
         WebView *seview = create_tab (false);
         seview->setUrl (QUrl::fromUserInput ("https://duckduckgo.com/?q="+text));
    });

    connect (view, &WebView::link_requested, [this] (const QString url)
    {
         WebView *seview = create_tab (false);
         seview->setUrl (QUrl::fromUserInput (url));
    });


    if (!background)
    {
        //view->setFocus();
        setCurrentWidget (view);
    }
        //tabBar()->setStyleSheet (QString ("QTabBar::tab { width: %1px; } ") .arg(view->size().height()/count()-1));
        //tabBar()->setStyleSheet (QString ("QTabBar::tab { height: %1px; width: %2px; }").arg (view->size().height()/count()-1).arg(tabBar()->minimumHeight()));
        //tabBar()->setTabButton (indexOf(view), SideTabs::RightSide, new QToolButton(this));
    view->setUrl (QUrl::fromUserInput("about:blank"));
    return view;
}



void TabWidget::close_tab (int index)
{
    WebView *view = qobject_cast<WebView*>(widget (index));
    if (view)
    {
        if (!count())
            create_tab ();

        QSaveFile file ("recently_closed.dat"); // For now only handles one...
        file.open (QIODevice::WriteOnly);
        QDataStream out (&file);
        out << *view->page()->history();
        file.commit();
        view->page()->deleteLater();
        view->deleteLater();
        removeTab(index);

        //setStyleSheet (QString ("QTabBar::tab { width: %1px; } ") .arg(size().width()/count()-1));
        //tabBar()->setStyleSheet (QString ("QTabBar::tab { height: %1px; width: %2px; }").arg (view->size().height()/count()-1).arg(tabBar()->minimumHeight()));

        emit update_session();
    }
    else // Not a web view! Settings and debug tabs fall into this category.
    {
        widget (index)->deleteLater();
        removeTab (index);
    }
}

// Very stubby.
void TabWidget::restore_tab()
{
    QFile file ("recently_closed.dat");
    file.open (QIODevice::ReadOnly);
    QDataStream in (&file);

    if (!in.atEnd())
    {
        WebView *view = create_tab();
        setCurrentWidget (view);
        while (!in.atEnd())
        {
            if (view)
            {
                in >> *view->page()->history();
            }
        }
    }
    file.close();
}

void TabWidget::set_url (const QUrl &url)
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
         view->setUrl (url);
        //view->setFocus();
    }
}

void TabWidget::back()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        view->back();
       // view->setFocus();
    }
}

void TabWidget::forward()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        view->forward();
       // view->setFocus();
    }
}

void TabWidget::refresh()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        /*UserScript jade_script;
        jade_script.load_from_file ("userscripts/jade_script.js");
        jade_script.setInjectionPoint (QWebEngineScript::DocumentReady);
        jade_script.setName ("Jade script");
        profile->scripts ()->remove (profile->scripts ()->findScript ("Jade script"));
        profile->scripts()->insert (jade_script);*/

        setTabIcon (currentIndex(), QIcon());
        view->reload();
        // view->setFocus();
    }
}

void TabWidget::refresh_no_cache()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        setTabIcon (currentIndex(), QIcon());
        view->triggerPageAction (QWebEnginePage::ReloadAndBypassCache);
    }
}

void TabWidget::open_in_background_tab (const QUrl &url)
{
    WebView *new_tab = create_tab (true);
    new_tab->setUrl(url);
}

//TODO: change to tab cloning. This implementation opens a page with no history. Though that conserves memory.
void TabWidget::back_in_new_tab ()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        if (!view->page()->history()->backItem().url().isEmpty())
            open_in_background_tab (view->page()->history()->backItem().url());
    }
}

void TabWidget::forward_in_new_tab ()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        if (!view->page()->history()->forwardItem().url().isEmpty())
            open_in_background_tab (view->page()->history()->forwardItem().url());
    }
}

void TabWidget::current_changed()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        view->setFocus();
        hide();
        show();
        /*if (view->page()->lifecycleState() != QWebEnginePage::LifecycleState::Active)
        {
            view->page()->setLifecycleState (QWebEnginePage::LifecycleState::Active);
        }

        //parentWidget ()->showNormal ();
        */
        emit url_changed (view->url());
    }
    else
    {
        emit url_changed (QUrl::fromUserInput(""));
        //widget (currentIndex())->setFocus();
    }

}

void TabWidget::suspend (int i)
{
    QSaveFile file (QString::number(i)+".dat");
    file.open (QIODevice::WriteOnly);
    QDataStream out (&file);

    WebView *view = qobject_cast<WebView*>(widget (i));
    if (view)
    {
        out << *view->page()->history();
    }
    file.commit();
    view->page()->deleteLater();
    view->deleteLater();
    //view->suspended = true;
}

void TabWidget::resume (int i)
{
    QFile file (QString::number(i)+".dat");
    file.open (QIODevice::ReadOnly);
    QDataStream in (&file);
    while (!in.atEnd())
    {
        WebView *view = qobject_cast<WebView*>(widget (i));
        //setCurrentWidget (view);
        //WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
        if (view)
        {
            in >> *view->page()->history();
            //view->suspended = false;
        }
    }
    file.close();
}

void TabWidget::save_state()
{
    QSaveFile file ("session.dat");
    file.open (QIODevice::WriteOnly);
    QDataStream out (&file);
    for (int i=0; i<=count(); ++i)
    {
        WebView *view = qobject_cast<WebView*>(widget (i));
        if (view)
        {
            out << *view->page()->history();
        }
    }
    file.commit();

    QSaveFile file2 ("popular_links.dat");
    file2.open (QIODevice::WriteOnly);
    QDataStream out2 (&file2);
    for (int i=0; i<model.rowCount (); ++i)
    {
        out2 << model.index (i, 0).data() << model.index (i, 1).data();
    }
    file2.commit();
}

void TabWidget::load_state()
{
    QFile file2 ("popular_links.dat");
    file2.open (QIODevice::ReadOnly);
    QDataStream in2 (&file2);
    while (!in2.atEnd())
    {
        QStandardItem *item1 = new QStandardItem;
        QStandardItem *item2 = new QStandardItem;
        QVariant v;
        in2 >> v;
        item1->setText (v.toString());
        in2 >> v;
        item2->setText (v.toString());
        QList <QStandardItem*> list;
        list.append (item1);
        list.append (item2);
        model.appendRow (list);
        list.clear();
    }
    file2.close();

    QFile file ("session.dat");
    file.open (QIODevice::ReadOnly);
    QDataStream in (&file);
    while (!in.atEnd())
    {
        WebView *view = create_tab();
        setCurrentWidget (view);
        //WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
        if (view)
        {
            in >> *view->page()->history();
        }
    }
    file.close();
}

void TabWidget::download (QWebEngineDownloadItem *download)
{
    Q_ASSERT (download && download->state() == QWebEngineDownloadItem::DownloadRequested);

    qDebug() << "Download mime:" << download->mimeType();

    if (!download->downloadFileName().contains (".")) // No extension?
    {
        QMimeDatabase db;
        QString suffix = db.mimeTypeForName (download->mimeType()).preferredSuffix();
        download->setDownloadFileName (download->downloadFileName() + "." + suffix);
    }

    // QString path = QFileDialog::getSaveFileName (this, tr ("Save as..."), download->downloadDirectory()+"/"+download->suggestedFileName());
    QString path = QFileDialog::getSaveFileName (this, tr("Save as"), QDir (download->downloadDirectory()).filePath (download->downloadFileName()));
    if (path.isEmpty())
        return;

    download->setDownloadDirectory(QFileInfo(path).path());
    download->setDownloadFileName(QFileInfo(path).fileName());

    download->accept();

}

SettingsTab *TabWidget::settings_tab()
{
    SettingsTab *view = new SettingsTab (this);
    insertTab (currentIndex()+1, view, tr ("Settings"));

    connect (view, &SettingsTab::reload_filters, this, &TabWidget::reload_filters);
    setTabIcon (currentIndex()+1, QIcon (QStringLiteral (":/icons/system")));
    setCurrentWidget (view);
    return view;
}


DebugTab *TabWidget::debug_tab()
{
    DebugTab *view = new DebugTab (this);
    insertTab (currentIndex()+1, view, tr("Debug"));

    connect (this, &TabWidget::print_to_debug_tab, [this, view] (const QString &text)
    {
        int index = indexOf (view);
        if (index != -1)
        {
            view->message (text);
        }
    });

    setTabIcon (currentIndex()+1, QIcon (QStringLiteral (":/icons/system")));
    setCurrentWidget (view);

    return view;
}