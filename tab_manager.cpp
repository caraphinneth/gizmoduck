#include <QToolButton>
#include <QHBoxLayout>
#include <QEvent>
#include <QSaveFile>
#include <QTabBar>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineScriptCollection>

#include "side_tabs.h"
#include "tab_manager.h"
#include "userscript.h"
#include "webpage.h"

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
    profile->settings()->setAttribute (QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    //profile->settings()->setAttribute (QWebEngineSettings::FocusOnNavigationEnabled, true);
    profile->settings()->setAttribute (QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
    profile->settings()->setAttribute (QWebEngineSettings::PlaybackRequiresUserGesture, true);
    profile->settings()->setAttribute (QWebEngineSettings::DnsPrefetchEnabled, true);
    //profile->settings()->setUnknownUrlSchemePolicy (QWebEngineSettings::AllowAllUnknownUrlSchemes);
    profile->settings()->setAttribute (QWebEngineSettings::AllowRunningInsecureContent, true);

    profile->settings()->setFontFamily (QWebEngineSettings::StandardFont, "Roboto");
    profile->settings()->setFontFamily (QWebEngineSettings::SerifFont, "Roboto");
    profile->settings()->setFontFamily (QWebEngineSettings::SansSerifFont, "Roboto");


    profile->setSpellCheckEnabled (false);

    // Remove Chromium orange outlines/ugly underlines
    UserScript custom_css;
    custom_css.load_from_file (":/scripts/custom_css");
    custom_css.setInjectionPoint (QWebEngineScript::DocumentReady);
    //custom_css.setRunsOnSubFrames (true);
    //custom_css.setWorldId(QWebEngineScript::ApplicationWorld);
    profile->scripts()->insert (custom_css);

    QStringList all_scripts;
    QDir script_dir ("./scripts");
    QStringList script_files = script_dir.entryList (QDir::Files);
    foreach (QString filename, script_files)
    {
        UserScript new_script;
        new_script.load_from_file ("./scripts/"+filename);
        new_script.setInjectionPoint (QWebEngineScript::DocumentReady);
        profile->scripts()->insert (new_script);
    }

    RequestFilter *request_filter = new RequestFilter;
    profile->setUrlRequestInterceptor (request_filter);

    //profile->setHttpUserAgent ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Gizmoduck/0.1.1 Chrome/61.0.3163.140 Safari/537.36");
    //profile->setHttpUserAgent ("Mozilla/5.0 (X11; Linux x86_64; rv:71.0) Gecko/20100101 Firefox/71.0");
    //profile->setHttpAcceptLanguage ("ru, en-gb, en-us;q=0.9, en;q=0.8|ru");

    // Set up download handler.
    // Should be moved to a separate widget later.
    connect (profile, &QWebEngineProfile::downloadRequested, this, &TabWidget::download);

    SideTabs *tabBar = new SideTabs (this, 162, 40);
    setTabBar (tabBar);

    //tabBar->setTabsClosable (true);
    tabBar->setSelectionBehaviorOnRemove (QTabBar::SelectPreviousTab);
    tabBar->setMovable (true);

    setTabPosition (East);
    tabBar->setContextMenuPolicy (Qt::CustomContextMenu);
    //setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Ignored);

    connect (tabBar, &QTabBar::tabCloseRequested, this, &TabWidget::close_page);

    /*
     * We don't need this with the new order.
    connect (tabBar, &QTabBar::tabBarDoubleClicked, [this]()
    {
        create_tab (false); // Lamdba does not honour default args?..
    });
    */

    setDocumentMode (true);
    //setElideMode (Qt::ElideRight);

    connect (this, &QTabWidget::currentChanged, this, &TabWidget::current_changed);
    connect (this, &TabWidget::update_session, this, &TabWidget::save_state);
    connect (this, &TabWidget::reload_filters, request_filter, &RequestFilter::ReloadLists);

    load_state();

    if (!count())
        create_tab ();

    connect (request_filter, &RequestFilter::debug_message, this, &TabWidget::print_to_debug_tab);
}


// Create a new web tab, link its events to interface where needed.
WebView *TabWidget::create_tab()
{
    WebView *view = new WebView (this);
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
            //emit update_session(); // This will recurse badly, don't do this.
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
           //emit update_session();
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
         set_url (QUrl::fromUserInput ("https://duckduckgo.com/?q="+text));
    });

    connect (view, &WebView::link_requested, [this] (const QString url)
    {
         set_url (QUrl::fromUserInput (url));
    });

    connect(view->page(), &QWebEnginePage::fullScreenRequested, this, &TabWidget::fullscreen_request);

       //tabBar()->setStyleSheet (QString ("QTabBar::tab { width: %1px; } ") .arg(view->size().height()/count()-1));
        //tabBar()->setStyleSheet (QString ("QTabBar::tab { height: %1px; width: %2px; }").arg (view->size().height()/count()-1).arg(tabBar()->minimumHeight()));
        //tabBar()->setTabButton (indexOf(view), SideTabs::RightSide, new QToolButton(this));
    //view->load (QUrl::fromUserInput("about:blank"));
    return view;
}

void TabWidget::close_page (int index)
{
    WebView *view = qobject_cast<WebView*>(widget (index));
    if (view)
    {
        QString host = host_views.key (view); // Not view->url().host()) since it might not be loaded.
        if (tab_groups.contains (host))
        {
            TabGroup* group = tab_groups.value (host);
            if (group->count()==1)
            {
                qDebug() << "Closing single-paged tab.";
                close_tab(index);
            }
            else
            {
                QSaveFile file ("recently_closed.dat"); // For now only handles one...
                file.open (QIODevice::WriteOnly);
                QDataStream out (&file);
                WebPage* p = qobject_cast<WebPage*>(view->page());
                qDebug() << "Saving page" << p->url().toString();
                out << *p->history();
                file.commit();
                qDebug() << "Removing allocation" << p->history()->currentItem().url().adjusted (QUrl::RemoveQuery).toString();
                group->remove (p->history()->currentItem().url().adjusted (QUrl::RemoveQuery).toString());
                p->deleteLater();
                if (!group->isEmpty())
                {
                    //Fixme: take a certain value. last() may not be accurate.
                    qDebug() << "Switching to page" << group->values().last()->url().toString();
                    view->setPage (group->values().last());
                }
                else
                {
                    qDebug() << "Group is empty, which should not happen! Discarding tab anyways";
                    tab_groups.remove (host);
                    host_views.remove (host);
                    delete group;
                    view->deleteLater();
                    removeTab(index);

                }
            }
            emit update_session();
        }
    }
    else // Not a web view! Settings and debug tabs fall into this category.
    {
        widget (index)->deleteLater();
        removeTab (index);
    }
}

void TabWidget::close_tab (int index)
{
    WebView *view = qobject_cast<WebView*>(widget (index));
    if (view)
    {
        if (!count())
            create_tab();

        QSaveFile file ("recently_closed.dat"); // For now only handles one...
        file.open (QIODevice::WriteOnly);
        QDataStream out (&file);
        QString host = host_views.key (view); // Not view->url().host()) since it might not be loaded.
        if (tab_groups.contains (host))
        {
            TabGroup* group = tab_groups.value (host);
            foreach (WebPage* p, *group)
            {
                //qDebug() << "Saving page"  << p->url().toString();
                out << *p->history();
                p->deleteLater();
            }
            file.commit();

            tab_groups.remove (host);
            host_views.remove (host);
            delete group;
        }
        else
            qDebug() << "Debug: host not grouped, OK if empty. Host:" << host;
        view->deleteLater();
        removeTab(index);

        //setStyleSheet (QString ("QTabBar::tab { width: %1px; } ") .arg(size().width()/count()-1));
        //tabBar()->setStyleSheet (QString ("QTabBar::tab { height: %1px; width: %2px; }").arg (view->size().height()/count()-1).arg(tabBar()->minimumHeight()));

    }
}

void TabWidget::restore_tab()
{
    QFile file ("recently_closed.dat");
    file.open (QIODevice::ReadOnly);
    QDataStream in (&file);

    if (!in.atEnd())
    {
        TabGroup* group;
        while (!in.atEnd())
        {
            WebPage* p = new WebPage(profile);
            in >> *p->history();
            p->setLifecycleState (QWebEnginePage::LifecycleState::Frozen);
            QString host = p->history()->currentItem().url().host();
            QString url = p->history()->currentItem().url().adjusted (QUrl::RemoveQuery).toString();
            qDebug() << "Loading page:"  << url;

            group = assign_tab_group (host);
            group->insert (url, p);
        }

        //WebView *view = create_tab();
        //host_views.insert (tab_groups.key (group), view);

        QStringList list;
        foreach (WebPage* p, *group)
        {
            list.append (p->history()->currentItem().url().toString());
            set_url (p->history()->currentItem().url().toString());
        }
        //qDebug() << "Creating view for host" << tab_groups.key (group);
        qDebug() << "Page list:" << list;
        //setCurrentWidget (view);

        //view->setPage (group->values().last());
    }
    file.close();
    file.remove();
}

void TabWidget::set_url (const QUrl &url, bool background)
{
    QString host = url.host();
    TabGroup* group = assign_tab_group (host);

    WebView* view;

    if (host_views.contains (host))
    {
        qDebug() << "Using existing view for host " << host;
        view = host_views.value (host);
    }
    else
    {
        qDebug() << "Creating view for host " << host;
        view = create_tab();
        host_views.insert (host, view);
    }

    if (group->contains (url.adjusted (QUrl::RemoveQuery).toString()))
    {
        qDebug() << "Loading existing page for url " << url.toString();
        WebPage* p = group->value (url.adjusted (QUrl::RemoveQuery).toString());
        view->setPage (p);
        // Query changed.
        if (p->history()->currentItem().url() != url)
        {
            view->load(url);
        }
    }
    else
    {
        qDebug() << "Allocating new page for url " << url.toString();
        WebPage* p = new WebPage (profile);
        group->insert (url.adjusted (QUrl::RemoveQuery).toString(), p);
        view->setPage (p);
        view->load (url);
        emit update_session();
    }
    if (!background)
        setCurrentWidget (view);
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

// FIXME: rework this, and history in general.
void TabWidget::back_in_new_tab()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        if (!view->page()->history()->backItem().url().isEmpty())
            set_url (view->page()->history()->backItem().url(), true);
    }
}

void TabWidget::forward_in_new_tab()
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        if (!view->page()->history()->forwardItem().url().isEmpty())
            set_url (view->page()->history()->forwardItem().url(), true);
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
        emit url_changed (view->url());
    }
    else
    {
        emit url_changed (QUrl::fromUserInput(""));
        //widget (currentIndex())->setFocus();
    }

}

void TabWidget::save_state()
{
    QSaveFile file ("session.dat");
    file.open (QIODevice::WriteOnly);
    QDataStream out (&file);

    // Use iterator if you need to access value()/values(), otherwise it makes en extra copy.
    TabGroups::const_iterator i;
    for (i = tab_groups.begin(); i != tab_groups.end(); ++i)
    //foreach (TabGroup* group, tab_groups)
    {
        qDebug() << "Saving group for host"  << i.key();
        TabGroup::const_iterator j;
        for (j = i.value()->begin(); j != i.value()->end(); ++j)
        //foreach (WebPage* p, *group)
        {
            WebPage* p = j.value();
            //qDebug() << "Saving page"  << p->url().toString();
            out << *p->history();
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
        WebPage* p = new WebPage(profile);
        in >> *p->history();
        p->setLifecycleState (QWebEnginePage::LifecycleState::Frozen);
        QString host = p->history()->currentItem().url().host();
        QString url = p->history()->currentItem().url().adjusted (QUrl::RemoveQuery).toString();
        qDebug() << "Loading page:"  << url;

        TabGroup* group = assign_tab_group (host);
        group->insert (url, p); // insert() replaces any possible dupes.
    }
    file.close();

    TabGroups::const_iterator i;
    for (i = tab_groups.begin(); i != tab_groups.end(); ++i)
    //foreach (TabGroup* group, tab_groups)
    {
        WebView* view = create_tab();
        host_views.insert (i.key(), view);

        qDebug() << "Creating view for host"  << i.key();

        emit view->iconChanged (QIcon (QStringLiteral (":/icons/freeze")));
        view->setPage (i.value()->values().last());
    }
}

void TabWidget::download (QWebEngineDownloadItem *download)
{
    Q_ASSERT (download && download->state() == QWebEngineDownloadItem::DownloadRequested);

    // qDebug() << "Download mime:" << download->mimeType();

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

void TabWidget::fullscreen_request (QWebEngineFullScreenRequest request)
{
    WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        if (request.toggleOn())
        {
            if (fullscreen)
                return;
            request.accept();
            fullscreen.reset(new FullScreenWindow (view));
        } else
        {
            if (!fullscreen)
                return;
            request.accept();
            fullscreen.reset();
        }
    }
}

TabGroup* TabWidget::assign_tab_group (QString host)
{
    if (tab_groups.contains (host))
    {
        TabGroup* group = tab_groups.value (host);
        return group;
    }
    else
    {
        TabGroup* group = new TabGroup;
        tab_groups.insert (host, group);
        return group;
    }
}

void TabWidget::wheelEvent (QWheelEvent *event)
{
    if (!tabBar()->geometry ().contains(event->position().x(), event->position().y()))
        QTabWidget::wheelEvent (event);
    else
    {
        WebView *view = qobject_cast<WebView*>(widget (currentIndex()));
        if (view)
        {
            QString host = host_views.key (view);
            if (tab_groups.contains (host))
            {
                TabGroup* group = tab_groups.value (host);
                WebPage* p = qobject_cast<WebPage*>(view->page());
                int  c = group->values().indexOf(p);
                if (event->angleDelta().y() > 0)
                {
                    if (group->values().first() == view->page())
                    {
                        if (currentIndex() != 0)
                            setCurrentIndex (currentIndex()-1);
                    }
                    else
                        view->setPage (group->values().at(c-1));
                }
                else if (event->angleDelta().y() < 0)
                {
                    if (group->values().last() == view->page())
                    {
                        if (currentIndex() != count()-1)
                            setCurrentIndex (currentIndex()+1);
                    }
                    else
                        view->setPage (group->values().at(c+1));
                }
            }
            else
            {
                if (event->angleDelta().y() > 0)
                {
                    if (currentIndex() != 0)
                        setCurrentIndex (currentIndex()-1);
                }
                else if (event->angleDelta().y() < 0)
                {
                    if (currentIndex() != count()-1)
                        setCurrentIndex (currentIndex()+1);
                }
            }
        }
        else
            QTabWidget::wheelEvent (event);
    }
}

void TabWidget::cleanup()
{
    foreach (TabGroup* group, tab_groups)
    {
        foreach (WebPage* p, *group)
        {
            p->deleteLater();
        }
    }
}