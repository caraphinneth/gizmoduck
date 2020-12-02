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
TabWidget::TabWidget (QWidget* parent): QTabWidget (parent)
{
    // Create QWebEngineProfile with the settings we prefer.
    profile = QWebEngineProfile::defaultProfile();
    //profile->settings()->setAttribute (QWebEngineSettings::ErrorPageEnabled, false); // Disable Chromium error pages for now (since they look ugly when adblocking).
    profile->settings()->setAttribute (QWebEngineSettings::FullScreenSupportEnabled, true);
    //profile->settings()->setAttribute (QWebEngineSettings::ScreenCaptureEnabled,true);
    profile->settings()->setAttribute (QWebEngineSettings::JavascriptCanOpenWindows, false);
    profile->settings()->setAttribute (QWebEngineSettings::PluginsEnabled, true);
    profile->settings()->setAttribute (QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    //profile->settings()->setAttribute (QWebEngineSettings::FocusOnNavigationEnabled, true);
    profile->settings()->setAttribute (QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
    profile->settings()->setAttribute (QWebEngineSettings::PlaybackRequiresUserGesture, false);
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
    foreach (const QString& filename, script_files)
    {
        UserScript new_script;
        new_script.load_from_file ("./scripts/"+filename);
        qDebug() << "Loading userscript" << "./scripts/"+filename;
        new_script.setInjectionPoint (QWebEngineScript::DocumentReady);
        profile->scripts()->insert (new_script);
    }


    //profile->setHttpUserAgent ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Gizmoduck/0.1.1 Chrome/61.0.3163.140 Safari/537.36");
   // profile->setHttpUserAgent ("Mozilla/5.0 (X11; Linux x86_64; rv:71.0) Gecko/20100101 Firefox/71.0");
    //profile->setHttpUserAgent ("Googlebot/2.1 (+http://www.google.com/bot.html)");
    //profile->setHttpAcceptLanguage ("ru, en-gb, en-us;q=0.9, en;q=0.8|ru");
    RequestFilter* request_filter = new RequestFilter (nullptr, profile->httpUserAgent());
    profile->setUrlRequestInterceptor (request_filter);

    // Set up download handler.
    // Should be moved to a separate widget later.
    connect (profile, &QWebEngineProfile::downloadRequested, this, &TabWidget::download);

    SideTabs* tabBar = new SideTabs (this, 162, 40);
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
    connect (this, &TabWidget::reload_filters, request_filter, &RequestFilter::ReloadLists);

    load_state();

    if (!count())
        set_url(QUrl::fromUserInput("duckduckgo.com"));

    autosave = new QTimer (this);
    connect (this, &TabWidget::update_session, this, [this]()
    {
        autosave->start (10000);
    });
    connect (autosave, &QTimer::timeout, this, &TabWidget::save_state);

    connect (request_filter, &RequestFilter::debug_message, this, &TabWidget::print_to_debug_tab);
}


// Create a new web tab, link its events to interface where needed.
WebView* TabWidget::create_tab (bool at_end)
{
    WebView* view = new WebView (this);
    view->setAttribute(Qt::WA_DontShowOnScreen);
    if (!at_end)
        insertTab (currentIndex()+1, view, tr("Untitled"));
    else
        addTab (view, tr("Untitled"));

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
    connect (view, &QWebEngineView::titleChanged, [this, view] (const QString& title)
    {
        int index = indexOf (view);
        // Work around the bug(?) with returning url sometimes on heavily updated titles (may cause flicker).
        if ((index != -1) && (!title.startsWith ("http")) && (title!="about:blank"))
        {
            setTabText (index, title);
            QList<QStandardItem*> vl (model.findItems (view->url().toString()));
            if (vl.isEmpty ())
            {
                if (!view->url().toString().contains (title))
                {
                    QStandardItem* item1 = new QStandardItem (view->url().toString());
                    QStandardItem* item2 = new QStandardItem (title);
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
                QList<QStandardItem*> list = model.takeRow (vl.at(0)->row());
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
    connect (view, &QWebEngineView::urlChanged, [this, view] (const QUrl& url)
    {
        int index = indexOf (view);
        if (index != -1)
        {
            tabBar()->setTabData (index, url);
        }
        if (currentIndex() == index)
           emit url_changed (url);
    });

    // Set tab icon according to favicon.
    connect (view, &QWebEngineView::iconChanged, [this, view] (const QIcon& icon)
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
    connect (view, &WebView::search_requested, [this] (const QString& text)
    {
         set_url (QUrl::fromUserInput ("https://duckduckgo.com/?q="+text));
    });

    connect (view, &WebView::link_requested, [this] (const QString& url, const bool background)
    {
         set_url (QUrl::fromUserInput (url), background);
    });

       //tabBar()->setStyleSheet (QString ("QTabBar::tab { width: %1px; } ") .arg(view->size().height()/count()-1));
        //tabBar()->setStyleSheet (QString ("QTabBar::tab { height: %1px; width: %2px; }").arg (view->size().height()/count()-1).arg(tabBar()->minimumHeight()));
        //tabBar()->setTabButton (indexOf(view), SideTabs::RightSide, new QToolButton(this));
    //view->load (QUrl::fromUserInput("about:blank"));
    return view;
}

void TabWidget::close_page (int index)
{
    WebView* view = qobject_cast<WebView*>(widget (index));
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
                emit debug_tabs_updated();
            }
            else
            {
                QSaveFile file ("recently_closed.dat"); // For now only handles one...
                file.open (QIODevice::WriteOnly);
                QDataStream out (&file);
                if (group->contains (view->url().toString()))//.adjusted (QUrl::RemoveQuery)
                {
                    WebPage* p = group->value (view->url().toString()); //qobject_cast<WebPage*>(view->page()); //.adjusted (QUrl::RemoveQuery)
                    qDebug() << "Local: saving page" << p->url().toString();
                    out << *p->history();
                    file.commit();
                    qDebug() << "Removing allocation" << p->history()->currentItem().url().toString();//.adjusted (QUrl::RemoveQuery)
                    group->remove (p->history()->currentItem().url().toString());//.adjusted (QUrl::RemoveQuery)
                    emit debug_tabs_updated();
                    disconnect (p, &WebPage::fullScreenRequested, this, nullptr);
                    disconnect (p, &WebPage::urlChanged, this, nullptr);
                    p->deleteLater();
                }
                else
                {
                        qDebug() << "Not in group (stray redirect?). Not deleting.";
                        //p = qobject_cast<WebPage*>(view->page());
                        //p.clear();
                }

                if (!group->isEmpty())
                {
                    bool found = false;
                    for (auto i=history.rbegin(); i!=history.rend(); ++i)
                    {
                        QString s = i->toString();//.adjusted (QUrl::RemoveQuery)
                        if (group->contains (s))
                        {
                            qDebug() << "Switching to page" << i->toString();
                            if (!group->value(s))
                            {
                                qDebug()<<"BUG: page is invalid!";
                            }
                            else
                            {
                                qDebug()<<"Appers to be valid.";
                                view->setPage (group->value(s));
                                history.add (*i);
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found)
                    {
                        qDebug() << "No previous tab in history or orphans onlny!";
                        auto last = group->end();
                        --last;
                        WebPage* last_page = last.value();
                        view->setPage (last_page);
                    }
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
    WebView* view = qobject_cast<WebView*>(widget (index));
    if (view)
    {
        QSaveFile file ("recently_closed.dat"); // For now only handles one...
        file.open (QIODevice::WriteOnly);
        QDataStream out (&file);
        QString host = host_views.key (view); // Not view->url().host()) since it might not be loaded.
        if (tab_groups.contains (host))
        {
            TabGroup* group = tab_groups.value (host);
            //for (auto i = group->begin (); i!=group->end(); ++i)
            foreach (const QString& i, group->order)
            {
                //qDebug() << "Saving page"  << p->url().toString();
                out << *group->value(i)->history();

                disconnect (group->value(i), &WebPage::fullScreenRequested, this, nullptr);
                disconnect (group->value(i), &WebPage::urlChanged, this, nullptr);

                group->value(i)->deleteLater();

                history.back();
            }
            file.commit();

            tab_groups.remove (host);
            host_views.remove (host);
            delete group;
        }
        else
            qDebug() << "Debug: host not grouped, OK if empty. Host:" << host;
        if (count()==1)
            set_url (QUrl::fromUserInput("duckduckgo.com"), true);
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
            WebPage* p (new WebPage(profile));
            in >> *p->history();
            p->setLifecycleState (QWebEnginePage::LifecycleState::Frozen);
            QString host = p->history()->currentItem().url().host();
            QString url = p->history()->currentItem().url().toString();//.adjusted (QUrl::RemoveQuery)
            qDebug() << "Loading page:"  << url;

            install_page_signal_handler (p);

            group = assign_tab_group (host);
            group->insert (url, p);

            WebView* view = host_views.value (host);
            view->setPage (p);
            emit debug_tabs_updated();
        }

        //WebView *view = create_tab();
        //host_views.insert (tab_groups.key (group), view);
        /*
        QStringList list;
        //for (auto i = group->begin (); i!=group->end(); ++i)
        foreach (const QString& i, group->order)
        {
            WebPage* p = group->value(i);
            list.append (p->history()->currentItem().url().toString());
            set_url (p->history()->currentItem().url().toString());
        }
        //qDebug() << "Creating view for host" << tab_groups.key (group);
        qDebug() << "Page list:" << list;
        //setCurrentWidget (view);

        //view->setPage (group->values().last());
        */
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
        view = host_views.value (host);
    }
    else
    {
        view = create_tab();
        host_views.insert (host, view);
    }

    QString key = url.toString();//.adjusted (QUrl::RemoveQuery)

    WebPage* p;
    if (!view->page()->url().isEmpty() && !background)
    {
        qDebug() << "Setting page" <<url.toString () <<"page"<<view->page()->url().toString ();
        view->page()->setUrl (url);
        history.add (url);
    }
    else
    {
        p = group->assign_page (key);
        emit debug_tabs_updated();

        view->setPage (p);
        p->setUrl (url);
        history.add (p->url());
        p->old_url = url;
        install_page_signal_handler (p);
    }

    // Query changed.
    /*
    if (p->history()->currentItem().url() != url)
    {
        p->setUrl (url);
    }*/

    setCurrentWidget (view);
}

void TabWidget::install_page_signal_handler (WebPage* p)
{
    connect (p, &WebPage::fullScreenRequested, this, &TabWidget::fullscreen_request, Qt::UniqueConnection);

    //disconnect (p.data(), &WebPage::urlChanged, this, nullptr);
    connect (p, &WebPage::urlChanged, [this, p](const QUrl& new_url)
    {
        QString final_host = new_url.host();
        QString final_url = new_url.toString();//.adjusted (QUrl::RemoveQuery)

        // history()-> backItem() does not always yield what you want, sadly.
        QString key = p->old_url.toString();//.adjusted (QUrl::RemoveQuery)
        QString host = p->old_url.host();

        TabGroup* group = assign_tab_group (host);
        WebView* view = host_views.value (host);

        qDebug()<<"Debug: existing page changed its URL to" << final_url;

        // A redirect happened, reattach page.
        if (final_host != host)
        {
            group->remove (key);
            emit debug_tabs_updated();

            if (group->isEmpty())
            {
                tab_groups.remove (host);
                host_views.remove (host);
                delete group;
                view->deleteLater();
                removeTab(indexOf (view));
            }
            else
            {
                view->setPage (group->values().last());
            }

            TabGroup* new_group = assign_tab_group (final_host);

            // A suitable group already exists.
            if (host_views.contains (final_host))
            {
                // If you're led to an existing page by redirect, prune the older version of the page.
                // Can do better?
                if (new_group->contains (final_url))
                {
                    if (new_group->value (final_url) != p)
                    {
                        qDebug() << "Deleting old page" << final_url;
                        new_group->value (final_url)->deleteLater();
                    }
                }
                WebView* new_view = host_views.value (final_host);

                new_group->insert (final_url, p);
                emit debug_tabs_updated();

                history.add (p->url());
                // FIXME: Do not activate widgets mindlessly, since this affects session load, too.
                //setCurrentWidget (new_view);
                new_view->setPage (p);
            }
            // Creating a new group.
            else
            {
                qDebug() << "Creating view for host " << final_host;

                WebView* v = create_tab();
                host_views.insert (final_host, v);
                new_group->insert (final_url, p);
                emit debug_tabs_updated();

                v->setPage (p);
                history.add (p->url());
                setCurrentWidget (v);
            }
        }
        // It's a same host, so probably JS engine being malefic.
        else if (final_url != key)
        {
            qDebug() << "Removing" << key <<",replacing with"<< final_url;

            //group->insert (final_url, p);
            //group->remove (key);
            group->replace (key, final_url, p);
            emit debug_tabs_updated();

            history.add (p->url());
        }
        else
        {
            qDebug() << "Equal" << key <<"and"<< final_url;
        }
        p->old_url = new_url;
    });
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
    WebView* view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        setTabIcon (currentIndex(), QIcon());
        view->triggerPageAction (QWebEnginePage::ReloadAndBypassCache);
    }
}

// FIXME: rework this, and history in general.
void TabWidget::back_in_new_tab()
{
    WebView* view = qobject_cast<WebView*>(widget (currentIndex()));
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
        history.add (view->page()->url());
        // This reacts to dragging. Fix that.
        //qDebug() << "Switched to page" << history.current->toString();
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
    autosave->stop();
    QSaveFile file ("session.dat");
    file.open (QIODevice::WriteOnly);
    QDataStream out (&file);

    // Use iterator if you need to access value()/values(), otherwise it makes en extra copy.
    //for (auto i = tab_groups.begin(); i != tab_groups.end(); ++i)
    //foreach (auto i, tab_groups.order)
    for (int i=0; i<=count(); ++i)
    {
        WebView* view = qobject_cast<WebView*>(widget (i));
        if (view)
        {
            qDebug() << "Saving group for host"  << host_views.key (view);
            //for (auto j = tab_groups.value(host_views.key (view))->begin(); j != tab_groups.value(host_views.key (view))->end(); ++j)
            TabGroup* group = tab_groups.value (host_views.key (view));
            foreach (auto j, group->order)
            {
                WebPage* p = group->value(j);

                if (p)
                {
                    qDebug() << "Global: saving page"  << p->url().toString();
                    out << *p->history();
                }
                else
                {
                    qDebug() << "BUG: page is invalid.";
                }
            }
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

    qDebug() <<"Session saved.";
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
        WebPage* p (new WebPage(profile));
        in >> *p->history();
        p->setLifecycleState (QWebEnginePage::LifecycleState::Frozen);
        QString host = p->history()->currentItem().url().host();
        QString url = p->history()->currentItem().url().toString(); //.adjusted (QUrl::RemoveQuery)
        qDebug() << "Loading page:"  << url;

        TabGroup* group = assign_tab_group (host);
        group->insert (url, p); // insert() replaces any possible dupes.

        history.add (p->history()->currentItem().url());
        install_page_signal_handler (p);
    }
    file.close();

    //for (auto i = tab_groups.begin(); i != tab_groups.end(); ++i)
    foreach (auto i, tab_groups.order)
    {
        WebView* view = create_tab (true);
        host_views.insert (i, view);

        //qDebug() << "Creating view for host"  << i;

        emit view->iconChanged (QIcon (QStringLiteral (":/icons/freeze")));
        //auto last = tab_groups.value(i)->end();
        TabGroup* group = tab_groups.value(i);
        auto last = group->value (group->order.back());
        //--last;
        //WebPage* last_page = last.value();
        view->setPage (last);
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


DebugTab* TabWidget::debug_tab()
{
    DebugTab* view = new DebugTab (this);
    insertTab (currentIndex()+1, view, tr("Debug"));

    connect (this, &TabWidget::print_to_debug_tab, [this, view] (const QString& text)
    {
        int index = indexOf (view);
        if (index != -1)
        {
            emit view->message_received (text);
        }
    });

    connect (this, &TabWidget::debug_tabs_updated, [this, view]()
    {
        int index = indexOf (view);
        if (index != -1)
        {
            emit view->redraw_tabs (tab_groups);
        }
    });

    setTabIcon (currentIndex()+1, QIcon (QStringLiteral (":/icons/system")));
    setCurrentWidget (view);

    emit debug_tabs_updated();

    return view;
}

void TabWidget::fullscreen_request (QWebEngineFullScreenRequest request)
{
    WebView* view = qobject_cast<WebView*>(widget (currentIndex()));
    if (view)
    {
        if (request.toggleOn())
        {
            if (fullscreen)
                return;
            request.accept();
            fullscreen.reset (new FullScreenWindow (view));
        } else
        {
            if (!fullscreen)
                return;
            request.accept();
            fullscreen.reset();
        }
    }
}

TabGroup* TabWidget::assign_tab_group (const QString& host)
{
    if (tab_groups.contains (host))
    {
        TabGroup* group = tab_groups.value (host);
        return group;
    }
    else
    {
        TabGroup* group = new TabGroup;
        group->profile = profile;
        tab_groups.insert (host, group);
        return group;
    }
}

void TabWidget::wheelEvent (QWheelEvent* event)
{
    if (!tabBar()->geometry().contains (event->position().x(), event->position().y()))
        QTabWidget::wheelEvent (event);
    else
    {
        WebView* view = qobject_cast<WebView*>(widget (currentIndex()));
        if (view)
        {
            QString host = host_views.key (view);
            if (tab_groups.contains (host))
            {
                TabGroup* group = tab_groups.value (host);
                //auto it = group->find (view->url().toString()); // Reaching for a page pointer here is a disaster, don't do that. .adjusted (QUrl::RemoveQuery)
                auto it = std::find (group->order.begin(), group->order.end(), view->url().toString());

                if (event->angleDelta().y() > 0)
                {
                    if (it == group->order.begin())
                    {
                        if (currentIndex() > 0)
                        {
                            setCurrentIndex (currentIndex()-1);
                            //WebView* v = qobject_cast<WebView*>(widget (currentIndex()));
                            //history.add (v->page()->url());
                            //qDebug() << "History list:" << history;
                        }
                    }
                    else
                    {
                        --it;
                        view->setPage (group->value (*it));
                        history.add (view->page()->url());
                        //qDebug() << "History list:" << history;
                    }
                }
                else if (event->angleDelta().y() < 0)
                {
                    auto last = group->order.end();
                    --last;
                    if (it == last)
                    {
                        if (currentIndex() < count()-1)
                        {
                            setCurrentIndex (currentIndex()+1);
                            //WebView* v = qobject_cast<WebView*>(widget (currentIndex()));
                            //history.add (v->page()->url());
                            //qDebug() << "History list:" << history;
                        }
                    }
                    else
                    {
                        ++it;
                        view->setPage (group->value (*it));
                        history.add (view->page()->url());
                        //qDebug() << "History list:" << history;
                    }
                }
            }
            else
                QTabWidget::wheelEvent (event);
        }
        else
        // Likely an utility tab.
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
}

void TabWidget::cleanup()
{
    save_state();
    for (auto i = tab_groups.begin(); i != tab_groups.end(); ++i)
    {
        TabGroup* group = i.value();
        for (auto j = group->begin(); j != group->end(); ++j)
        {
            j.value()->deleteLater();
        }
    }
}

WebPage* TabWidget::page_back (TabGroup* group)
{
    for (auto i=history.rbegin(); i!=history.rend(); ++i)
    {
        QString s = i->toString(); //.adjusted (QUrl::RemoveQuery)
        if (group->contains (s))
        {
            qDebug() << "Switching to page" << i->toString();
            history.add (*i);
            return (group->value(s));
        }
    }
    return nullptr;
}

WebPage* TabWidget::page_forward(TabGroup* group)
{
    for (auto i=history.rbegin(); i!=history.rend(); ++i)
    {
        QString s = i->toString(); //.adjusted (QUrl::RemoveQuery)
        if (group->contains (s))
        {
            qDebug() << "Switching to page" << i->toString();
            history.add (*i);
            return (group->value(s));
        }
    }
    return nullptr;
}