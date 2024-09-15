#include <QMimeDatabase>
#include <QSaveFile>
#include <QStackedWidget>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineScriptCollection>

#include "side_tabs.h"
#include "tab_manager.h"
#include "userscript.h"
#include "webpage.h"


// The tab widget with web views and more for tab contents. Navigation functions are also handled by this one.
TabWidget::TabWidget (QWidget* parent): QStackedWidget (parent)
{
    // Create QWebEngineProfile with the settings we prefer.
    profile = new QWebEngineProfile("Default");
    //profile->settings()->setAttribute (QWebEngineSettings::ErrorPageEnabled, false); // Disable Chromium error pages (they may look ugly when adblocking).
    profile->settings()->setAttribute (QWebEngineSettings::FullScreenSupportEnabled, true);
    //profile->settings()->setAttribute (QWebEngineSettings::ScreenCaptureEnabled,true);
    profile->settings()->setAttribute (QWebEngineSettings::JavascriptCanOpenWindows, true);
    profile->settings()->setAttribute (QWebEngineSettings::JavascriptCanAccessClipboard, true);
    profile->settings()->setAttribute (QWebEngineSettings::JavascriptCanPaste, true);
    //profile->settings()->setAttribute (QWebEngineSettings::PluginsEnabled, true);
    profile->settings()->setAttribute (QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    profile->settings()->setAttribute (QWebEngineSettings::FocusOnNavigationEnabled, true);
    profile->settings()->setAttribute (QWebEngineSettings::TouchIconsEnabled, true);
    profile->settings()->setAttribute (QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
    profile->settings()->setAttribute (QWebEngineSettings::PlaybackRequiresUserGesture, false);
    profile->settings()->setAttribute (QWebEngineSettings::DnsPrefetchEnabled, true);
    profile->settings()->setUnknownUrlSchemePolicy (QWebEngineSettings::AllowAllUnknownUrlSchemes);
    profile->settings()->setAttribute (QWebEngineSettings::AllowRunningInsecureContent, true);

    profile->settings()->setFontFamily (QWebEngineSettings::StandardFont, "Noto Sans SemiCondensed");
    profile->settings()->setFontFamily (QWebEngineSettings::SerifFont, "Noto Sans SemiCondensed");
    profile->settings()->setFontFamily (QWebEngineSettings::SansSerifFont, "Noto Sans SemiCondensed");
    profile->settings()->setFontFamily (QWebEngineSettings::FixedFont, "ClassicConsoleNeue Nerd Font");
    profile->settings()->setFontSize(QWebEngineSettings::MinimumFontSize, 12);
    profile->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, 16);
    profile->settings()->setFontSize(QWebEngineSettings::DefaultFixedFontSize, 16);

    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    profile->setSpellCheckEnabled (false);

    // Remove Chromium orange outlines/ugly underlines
    UserScript custom_css;
    custom_css.load_from_file (":/scripts/custom_css");
    custom_css.setInjectionPoint (QWebEngineScript::DocumentReady);
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

    profile->setHttpUserAgent ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
    //profile->setHttpUserAgent ("Mozilla/5.0 (X11; Linux x86_64; rv:71.0) Gecko/20100101 Firefox/71.0");
    //profile->setHttpUserAgent ("Googlebot/2.1 (+http://www.google.com/bot.html)");
    //profile->setHttpAcceptLanguage ("ru, en-gb, en-us;q=0.9, en;q=0.8|ru");
    RequestFilter* request_filter = new RequestFilter (this, profile->httpUserAgent());
    profile->setUrlRequestInterceptor (request_filter);

    // Set up download handler.
    // Should be moved to a separate widget later.
    connect (profile, &QWebEngineProfile::downloadRequested, this, &TabWidget::download);

    tabBar = new SideTabs (this, 162, 44);

    connect (tabBar->model(), &QAbstractItemModel::rowsRemoved, [this](const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
    {
        int selected = tabBar->selectionModel()->selectedIndexes().first().row();
        // DnD selection workaround.
        //qDebug()<<"Current index:"<<indexOf (currentWidget());
        if (currentWidget() != tabBar->widget (selected))
            tabBar->setCurrentIndex (tabBar->model()->index (tabBar->indexOf (currentWidget()), 0));
    });

    //tabBar->setSelectionBehaviorOnRemove (QTabBar::SelectPreviousTab);

    connect (tabBar, &SideTabs::pressed, [this](const QModelIndex& index)
    {
        tabBar->setCurrentIndex(index);
        emit current_changed (index.row());
    });



    connect (tabBar, &SideTabs::temp_pass_wheel_event, [this](QWheelEvent* event)
    {
        if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
        {
            QString host = host_views.key(view);
            if (tab_groups.contains(host))
            {
                TabGroup* group = tab_groups.value(host);
                int i = group->order.indexOf(view->url().toString());

                if (event->angleDelta().y() > 0)
                {
                    if (i == 0)
                    {
                        if (tabBar->currentIndex().row() > 0)
                        {
                            //setCurrentWidget (tabBar->widget (tabBar->currentIndex().row()-1));
                            QModelIndex index = tabBar->model()->index(tabBar->currentIndex().row()-1, 0);
                            tabBar->setCurrentIndex(index);
                            emit current_changed(index.row());
                            //history.add (view->page()->url());
                            //qDebug() << "History list:" << history;
                        }
                    }
                    else
                    {
                        --i;
                        qDebug() << "Trying index" << i;
                        set_page(view, group->get_page(group->order.at(i)));
                        history.add(view->page()->url());
                        // Ditto.
                    }
                }
                else if (event->angleDelta().y() < 0)
                {
                    if (i == group->order.size()-1)
                    {
                        if (tabBar->currentIndex().row() < count()-1) // maybe don't mix
                        {
                            QModelIndex index = tabBar->model()->index (tabBar->currentIndex().row()+1, 0);
                            tabBar->setCurrentIndex (index);
                            emit current_changed (index.row());
                            //history.add (view->page()->url());
                            //qDebug() << "History list:" << history;
                        }
                    }
                    else
                    {
                        ++i;
                        qDebug() << "Trying index" << i;
                        set_page(view, group->get_page(group->order.at(i)));
                        history.add(view->page()->url());
                        /*
                        TODO: if bugs persist, consider checking the result and removing key.
                        group->remove(group->order.at(i));
                        */
                    }
                }
            }
            else
            {
                qDebug() << "Rolled wheel on a tab which is not grouped! Shouldn't happen?";
                QStackedWidget::wheelEvent (event);
            }
        }
        else
        // Likely an utility tab.
        {
            if (event->angleDelta().y() > 0)
            {
                if (tabBar->currentIndex().row() != 0)
                {
                    QModelIndex index = tabBar->model()->index(tabBar->currentIndex().row()-1, 0);
                    tabBar->setCurrentIndex(index);
                    emit current_changed(index.row());
                }
            }
            else if (event->angleDelta().y() < 0)
            {
                if (tabBar->currentIndex().row() != count()-1)
                {
                    QModelIndex index = tabBar->model()->index(tabBar->currentIndex().row()+1, 0);
                    tabBar->setCurrentIndex(index);
                    emit current_changed(index.row());
                }
            }
        }
    });

    connect(this, &TabWidget::reload_filters, request_filter, &RequestFilter::ReloadLists);

    process_manager = new ProcessManager(this);
    process_tab = new ProcessTab(this);

    connect(process_manager, &ProcessManager::started, process_tab, &ProcessTab::add_process);
    connect(process_manager, &ProcessManager::output_received, process_tab, &ProcessTab::update_process);
    connect(process_manager, &ProcessManager::finished, process_tab, &ProcessTab::finish_process);

    load_state();

    if (!count())
        set_url(QUrl::fromUserInput("duckduckgo.com"));

    autosave = new QTimer(this);
    connect(this, &TabWidget::update_session, this, [this]()
    {
        autosave->start(10000);
    });
    connect (autosave, &QTimer::timeout, this, &TabWidget::save_state);

    connect (request_filter, &RequestFilter::debug_message, this, &TabWidget::print_to_debug_tab);
}

void TabWidget::set_page(WebView* view, const QWeakPointer<WebPage> page)
{
    if (!view)
    {
        qDebug() << "The view has gone out while setting page...";
        return;
    }

    if (auto lock = page.lock())
    {
        view->setPage(lock.data());
    }
    else
    {
        qDebug() << "BUG: page requested is null or deleted!";
    }
}

void TabWidget::setTabIcon(int index, const QIcon& icon)
{
    QModelIndex model_index = tabBar->model()->index(index, 0);
    tabBar->model()->setData(model_index, icon, Qt::DecorationRole);
}

void TabWidget::setTabText(int index, const QString& text)
{
    QModelIndex model_index = tabBar->model()->index(index, 0);
    tabBar->model()->setData(model_index, text);
}

// Create a new web tab, link its events to interface where needed.
WebView* TabWidget::create_tab(bool at_end)
{
    WebView* view = new WebView(profile, this);
    //view->setAttribute (Qt::WA_DontShowOnScreen);

    //The model should take the ownership of the item.
    QStandardItem* item = new QStandardItem(view->title());
    item->setData(reinterpret_cast<quintptr>(view), Qt::UserRole);

    // Forbid dropping 'into' a tab header. Not to be confused with the one in ListView.
    item->setDropEnabled(false);

    QStandardItemModel* model = static_cast<QStandardItemModel*>(tabBar->model());

    if (!at_end)
    {
        insertWidget(currentIndex()+1, view);
        if (!tabBar->selectionModel()->selectedIndexes().isEmpty())
        {
            int row = tabBar->selectionModel()->selectedIndexes().first().row();
            model->insertRow(row+1, item);
        }
        else
            model->appendRow(item);
    }
    else
    {
        addWidget(view);
        model->appendRow(item);
    }

    int index = tabBar->indexOf(view);
    if (index != -1)
    {
        setTabIcon(index, QIcon(QStringLiteral(":/icons/gizmoduck")));
    }

    // Autoassign tab title.
    connect(view, &QWebEngineView::titleChanged, [this, view] (const QString& title)
    {
        int index = tabBar->indexOf(view);
        // Work around the bug(?) with returning url sometimes on heavily updated titles (may cause flicker).
        if ((index != -1) && (!title.startsWith("http")) && (title!="about:blank"))
        {
            setTabText(index, title);
            QList<QStandardItem*> vl(suggestions.findItems(view->url().toString()));
            if (vl.isEmpty ())
            {
                if (!view->url().toString().contains(title))
                {
                    QStandardItem* item1 = new QStandardItem(view->url().toString());
                    QStandardItem* item2 = new QStandardItem(title);
                    QList<QStandardItem*> list;
                    list.append(item1);
                    list.append(item2);
                    suggestions.insertRow(0, list);
                    if (suggestions.rowCount()>5000)
                        suggestions.setRowCount (5000); // Data is discarded.
                    list.clear();
                }
            }
            else
            {
                suggestions.moveRow(QModelIndex(), suggestions.indexFromItem(vl.at(0)).row(), QModelIndex(), 0);
            }
            vl.clear();
        }
        //if (currentIndex() == index)
          //  emit title_changed (title);
    });

    // Better than linking at top level, since url may change with both navigation and tab switching.
    connect(view, &QWebEngineView::urlChanged, [this, view] (const QUrl& url)
    {
        int index = tabBar->indexOf(view);

        //view->page()->profile()->requestIconForIconURL(url, 16, view->page()->WebPage::getFavicon);

        if (currentIndex() == index)
            emit url_changed(url);
    });

    // Set tab icon according to favicon.
    connect(view, &QWebEngineView::iconChanged, [this, view] (const QIcon& icon)
    {
        int index = tabBar->indexOf(view);
        if (!icon.isNull() && (index != -1))
        {
            setTabIcon(index, icon);
        }
    });

    connect(view, &QWebEngineView::loadFinished, [this, view]()
    {
        int index = tabBar->indexOf (view);

        if (index != -1)
        {
            profile->requestIconForPageURL(view->url(), 16, [this, index, view](const QIcon& icon, const QUrl&, const QUrl&)
            {
                setTabIcon(index, icon.isNull() ? QIcon(QStringLiteral(":/icons/gizmoduck")) : icon);
                view->page()->setProperty("icon", icon);
            });
            emit update_session();
        }
    });

    // Install a handler for the context menu action.
    connect(view, &WebView::search_requested, [this](const QString& text)
    {
         set_url(QUrl::fromUserInput("https://duckduckgo.com/?q="+text));
    });

    connect(view, &WebView::link_requested, [this](const QString& url, const bool background)
    {
         set_url(QUrl::fromUserInput(url), background);
    });

    connect (view, &WebView::process_requested, process_manager, &ProcessManager::start_process);

    /*
    connect (view, &WebView::went_to_sleep, [this, view]()
    {
        int index = tabBar->indexOf(view);

        if ((index!=-1) && (index < tabBar->model()->rowCount() - 1))
        {
            tabBar->model()->moveRow(tabBar->model()->index(index, 0), index, tabBar->model()->index(index+1, 0), 0);
        }
    });
    */

    return view;
}

void TabWidget::close_page(int index)
{
    if (auto view = qobject_cast<WebView*>(tabBar->widget(index)))
    {
        QString host = host_views.key(view); // Not view->url().host()) since it might not be loaded.
        // QString host = view->history()->currentItem().url().host();
        if (tab_groups.contains(host))
        {
            TabGroup* group = tab_groups.value(host);
            if ((group->count() == 1) & (count() == 1))
            {
                // Do not remove the only tab.
            }
            else if (group->count()==1)
            {
                qDebug() << "Closing single-paged tab.";
                close_tab(index);
                emit debug_tabs_updated();
            }
            else
            {
                QString url = view->history()->currentItem().url().toString();

                bool found = false;
                for (auto i=history.rbegin(); i!=history.rend(); ++i)
                {
                    QString s = i->toString();

                    // FIXME: That's a rather fragile condition!
                    if ((group->contains(s)) && (s != url))
                    {
                        qDebug()<< "Switching to page" << s;
                        set_page(view, group->get_page(s));

                        history.add(*i);
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    qDebug() << "No previous tab in history or orphans only!";
                    auto last = std::prev(group->end());
                    while(!(last.value()))
                    {
                        qDebug() << "BUG: page is invalid!";
                        last = std::prev(last);
                    }
                    set_page(view, last.value());
                }

                if (group->contains(url))
                {
                    if (auto page = group->get_page(url).lock())
                    {
                        QSaveFile file("recently_closed.dat"); // For now only handles one...
                        file.open(QIODevice::WriteOnly);
                        QDataStream out(&file);
                        qDebug() << "Local: saving page" << url;
                        out << *page->history();
                        file.commit();
                        qDebug() << "Marking" << url << "for deletion."; //.adjusted (QUrl::RemoveQuery)
                        page.data()->disconnect();
                        group->remove(url);//.adjusted (QUrl::RemoveQuery)
                        emit debug_tabs_updated();
                    }
                    else
                    {
                        qDebug() << "BUG: page has a group assigned, but a smart pointer object no longer exists!" << url;
                    }
                }
                else
                {
                    qDebug() << "BUG: page has no group assigned (stray redirect?). Not deleting." << url;
                    //QSharedPointer<WebPage> p (view->page());
                    //p.clear();
                }
            }
            emit update_session();
        }
        else
        {
            qDebug() << "Host not in tab groups (WTF?).";
        }
    }
    else // Not a web view! Settings and debug tabs fall into this category.
    {
        int n = qMax(0, tabBar->currentIndex().row()-1);
        QModelIndex i = tabBar->model()->index(n, 0);
        tabBar->setCurrentIndex(i);
        emit currentChanged(i.row());

        if (!qobject_cast<ProcessTab*>(tabBar->widget(index)))
        {
            tabBar->widget(index)->disconnect();
            //tabBar->widget(index)->deleteLater();
        }
        removeWidget(tabBar->widget(index));
        tabBar->model()->removeRow(index);
    }
}

void TabWidget::close_tab(int index)
{
    if (auto view = qobject_cast<WebView*>(tabBar->widget(index)))
    {
        QSaveFile file ("recently_closed.dat"); // For now only handles one...
        file.open (QIODevice::WriteOnly);
        QDataStream out (&file);
        //QString host = host_views.key (view); // Not view->url().host()) since it might not be loaded.
        QString host = view->history()->currentItem().url().host();

        // Do not disconnect the active page.
        view->setPage(nullptr);

        if (tab_groups.contains(host))
        {
            TabGroup* group = tab_groups.value (host);
            //for (auto i = group->begin (); i!=group->end(); ++i)
            foreach (const QString& i, group->order)
            {
                if (auto page = group->get_page(i).lock())
                {
                    qDebug() << "Saving page"  << i;
                    out << *page->history();
                    page->disconnect();
                    group->remove(i);
                    history.back();
                }
                else
                {
                    qDebug() << "BUG: null or deleted page:" << i;
                }
            }
            file.commit();

            view->disconnect();
            tab_groups.remove (host);
            host_views.remove (host);
            delete group;
        }
        else
            qDebug() << "Debug: host not grouped, OK if empty. Host:" << host;

        if (count() == 0)
            set_url (QUrl::fromUserInput("duckduckgo.com"), true);
        int n = qMax (0, tabBar->currentIndex().row() - 1);
        QModelIndex i = tabBar->model()->index (n, 0);
        tabBar->setCurrentIndex (i);
        emit current_changed (i.row());

        removeWidget (tabBar->widget (index));
        tabBar->model()->removeRow (index);
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
            QSharedPointer<WebPage> p(new WebPage(profile));
            in >> *p->history();
            p->setLifecycleState (QWebEnginePage::LifecycleState::Frozen);
            QString host = p->history()->currentItem().url().host();
            QString url = p->history()->currentItem().url().toString();//.adjusted (QUrl::RemoveQuery)

            group = assign_tab_group(host);
            group->insert(url, p);

            connect(p.data(), &WebPage::fullScreenRequested, this, &TabWidget::fullscreen_request, Qt::UniqueConnection);
            connect(p.data(), &WebPage::url_changed, this, &TabWidget::manage_page, Qt::UniqueConnection);

            WebView* view = assign_host_view(host);
            set_page(view, p);
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

        set_page(view, group->values().last());
        */
    }

    file.close();
    file.remove();
}

void TabWidget::set_url(const QUrl& url, bool background)
{
    QString host = url.host();
    TabGroup* group = assign_tab_group(host);
    WebView* view;
    if (host_views.contains(host))
    {        
        view = host_views.value(host);
    }
    else
    {
        view = create_tab();
        host_views.insert(host, view);
    }

    QString key = url.toString();

    if (!view->page()->url().isEmpty()  && !background && (view == qobject_cast<WebView*>(widget(currentIndex()))))
    // TODO: that's some weird condition... Trace the source and simplify this!
    {
        qDebug() << "Setting page" << url.toString() << ", initial URL" <<view->page()->url().toString();
        view->page()->setUrl(url);
        history.add(url);
        if (auto page = group->get_page(key).lock())
        {
            page->old_url = url;
        }
    }
    else
    {
        qDebug() << "If you see this, page is about it be assigned.";
        int i = group->order.indexOf(view->url().toString());

        if (auto page = group->get_page(key, i).lock())
        {
            emit debug_tabs_updated();
            page->setUrl(url);
            set_page(view, page);
            history.add(page->url());
            page->old_url = url;
            // QA: study the queued/direct behaviour for unique connections,
            connect(page.data(), &WebPage::fullScreenRequested, this, &TabWidget::fullscreen_request, Qt::UniqueConnection);
            connect(page.data(), &WebPage::url_changed, this, &TabWidget::manage_page, Qt::UniqueConnection);
        }
        else
        {
            qDebug() << "BUG: unable to access the page " << key;
        }
    }

    QModelIndex index = tabBar->model()->index(tabBar->indexOf(view), 0);
    tabBar->setCurrentIndex(index);
    emit current_changed(index.row());
}

void TabWidget::manage_page(const QUrl& url_old, const QUrl& url_new)
{
    // history()->backItem() does not always yield what you want, sadly.
    QString old_host = url_old.host();
    QString new_host = url_new.host();
    QString old_url = url_old.toString();
    QString new_url = url_new.toString();

    TabGroup* old_group = assign_tab_group(old_host);
    WebView* old_view  = host_views.value(old_host);

    qDebug() << "Existing page" << old_url << "changed its URL to" << new_url;

    // QA: THERE SHOULD BE NO OTHER PLACES MODIFYING THE PAGE TABLE.
    // The page that changed the URL is in its old_group by now.
    // Lock the object so that it won't get hurt by removal from old_group (QA: connections might keep it alive actually).
    if (auto page = old_group->get_page(old_url).lock())
    {
        if (old_host != new_host)
        // A redirect happened, reattach page.
        // EXPERIMENTAL: intercept all sorts of redirection here, rather than in acceptNavigationRequest().
        {
            old_group->remove(old_url);
            emit debug_tabs_updated();

            if (old_group->isEmpty())
            // That was the only page in a group, dropping group...
            // TODO: smartify this, too.
            {
                tab_groups.remove(old_host);
                host_views.remove(old_host);
                delete old_group;
                int n = qMax(0, tabBar->currentIndex().row()-1);
                QModelIndex index = tabBar->model()->index(n, 0);
                tabBar->setCurrentIndex(index);
                emit current_changed(index.row());
                if(old_view)
                {
                    old_view->disconnect();
                    removeWidget(old_view);
                    tabBar->model()->removeRow(tabBar->indexOf(old_view));
                    old_view->deleteLater();
                }
            }
            else if (old_view)
            // Page should now go into another group, but there's still other pages in the old one.
            // Set the view to the last one.
            {
                auto last = old_group->order.last();
                qDebug() << "Active page moved to a new group, activating" << last;
                set_page(old_view, old_group->get_page(last));
            }

            // QA: moving this up will barely create any overhead?
            TabGroup* new_group = assign_tab_group(new_host);

            // A suitable group already exists.
            if (host_views.contains(new_host))
            {
                // If you're led to an existing page by redirect, prune the older version of the page.
                // Can do better?
                if (new_group->contains(new_url))
                {
                    if (auto old_page = new_group->get_page(new_url).lock())
                    {
                        if (old_page != page)
                        {
                            qDebug() << "Marking old page for deletion:" << new_url;
                            old_page.data()->disconnect();
                            // clear() should not be needed.
                        }
                        else
                        {
                            qDebug() << "BUG: page changes its URL to the same one while being still in the wrong group:" << new_url;
                            qDebug() << "You know, if this actually happens, things are kinda screwed up.";
                        }
                    }
                }
                WebView* new_view = host_views.value(new_host);

                // old_page, if any, should have zero references after this and get deleted.
                new_group->insert(new_url, page);
                // new_group now has a reference to the page.
                emit debug_tabs_updated();

                // QA: consistency?
                history.add(page->url());
                // QA: do not activate widgets mindlessly, since this affects session load, too.
                //setCurrentWidget(new_view);

                QModelIndex index = tabBar->model()->index(tabBar->indexOf(new_view), 0);
                tabBar->setCurrentIndex(index);
                emit current_changed(index.row());

                set_page(new_view, page);
            }
            else
            // A freshly created group, needs a view.
            {
                qDebug() << "Creating view for host:" << new_host;

                WebView* v = create_tab();
                host_views.insert(new_host, v);
                new_group->insert(new_url, page);
                emit debug_tabs_updated();
                set_page(v, page);
                history.add(page->url());

                QModelIndex index = tabBar->model()->index(tabBar->indexOf(v), 0);
                tabBar->setCurrentIndex(index);
                emit current_changed(index.row());
            }
        }
        else if ((new_url != old_url) & (!new_url.startsWith("file:")))
        // URL changed within the same host. Replacing the page table key.
        {
            qDebug() << "Replacing URL.";
            old_group->replace(old_url, new_url, page);
            emit debug_tabs_updated();
            history.add(page->url());
        }
        else
        {
            qDebug() << "WARNING: redirect to the same page (WTF):" << new_url;
        }
        // TODO: signal this.
        page->old_url = url_new;
    }
    else
    {
        qDebug() << "WARNING: the page changed its URL but was deleted before we could handle it.";
    }
}

void TabWidget::back()
{
    if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
    {
        view->back();
       // view->setFocus();
    }
}

void TabWidget::forward()
{
    if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
    {
        view->forward();
       // view->setFocus();
    }
}

void TabWidget::refresh()
{
    if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
    {
        /*UserScript jade_script;
        jade_script.load_from_file ("userscripts/jade_script.js");
        jade_script.setInjectionPoint (QWebEngineScript::DocumentReady);
        jade_script.setName ("Jade script");
        profile->scripts ()->remove (profile->scripts()->findScript ("Jade script"));
        profile->scripts()->insert (jade_script);*/

        // setTabIcon (tabBar->indexOf(view), QIcon());
        view->reload();
        // view->setFocus();
    }
}

void TabWidget::refresh_no_cache()
{
    if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
    {
        //setTabIcon (tabBar->indexOf(view), QIcon());
        view->triggerPageAction (QWebEnginePage::ReloadAndBypassCache);
    }
}

// FIXME: rework this, and history in general.
void TabWidget::back_in_new_tab()
{
    if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
    {
        if (!view->page()->history()->backItem().url().isEmpty())
            set_url (view->page()->history()->backItem().url(), true);
    }
}

void TabWidget::forward_in_new_tab()
{
    if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
    {
        if (!view->page()->history()->forwardItem().url().isEmpty())
            set_url (view->page()->history()->forwardItem().url(), true);
    }
}

void TabWidget::current_changed(int index)
{
    TabWidget::setCurrentWidget(tabBar->widget(index));

    if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
    {
        view->setFocus();
        history.add(view->page()->url());
        emit url_changed(view->url());
    }
    else
    {
        emit url_changed(QUrl::fromUserInput(""));
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
        WebView* view = qobject_cast<WebView*>(tabBar->widget(i));
        if (view)
        {
            //qDebug() << "Saving group for host"  << host_views.key (view);
            //for (auto j = tab_groups.value(host_views.key (view))->begin(); j != tab_groups.value(host_views.key(view))->end(); ++j)
            TabGroup* group = tab_groups.value(host_views.key(view));
            foreach (auto j, group->order)
            {
                if (auto page = group->get_page(j).lock())
                {
                    out << *page->history();
                }
                else
                {
                    qDebug() << "BUG: page is invalid:" << j;
                    group->remove(j);
                }
            }
        }
    }
    file.commit();

    QSaveFile file2 ("popular_links.dat");
    file2.open (QIODevice::WriteOnly);
    QDataStream out2 (&file2);
    for (int i=0; i<suggestions.rowCount (); ++i)
    {
        out2 << suggestions.index (i, 0).data() << suggestions.index (i, 1).data();
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
        QStandardItem* item1 = new QStandardItem;
        QStandardItem* item2 = new QStandardItem;
        QVariant v;
        in2 >> v;
        item1->setText (v.toString());
        in2 >> v;
        item2->setText (v.toString());
        QList <QStandardItem*> list;
        list.append (item1);
        list.append (item2);
        suggestions.appendRow (list);
        list.clear();
    }
    file2.close();

    QFile file ("session.dat");
    file.open (QIODevice::ReadOnly);
    QDataStream in (&file);
    while (!in.atEnd())
    {
        // Not checking objects here, because, like, what the fuck could screw the memory this much.
        QSharedPointer<WebPage> p = QSharedPointer<WebPage>(new WebPage(profile));
        in >> *p->history();
        //p->setLifecycleState (QWebEnginePage::LifecycleState::Frozen);
        QString host = p->history()->currentItem().url().host();
        QUrl url = p->history()->currentItem().url();
        qDebug() << "Loading page:" << url;
        p->old_url = url;
        TabGroup* group = assign_tab_group(host);
        group->insert(url.toString(), p); // insert() replaces any possible dupes.
        history.add(url);
        connect(p.data(), &WebPage::fullScreenRequested, this, &TabWidget::fullscreen_request, Qt::UniqueConnection);
        connect(p.data(), &WebPage::url_changed, this, &TabWidget::manage_page, Qt::UniqueConnection);
        /*
        profile->requestIconForPageURL(url, 16, [this, p](const QIcon &icon, const QUrl, const QUrl)
        {
            if (!icon.isNull())
            {
                emit p->iconChanged(icon);
            }
        });
        */
    }
    file.close();

    foreach (auto i, tab_groups.order)
    {
        WebView* view = create_tab(true);
        host_views.insert(i, view);

        //setTabIcon(tabBar->indexOf(view), QIcon(QStringLiteral (":/icons/freeze")));
        TabGroup* group = tab_groups.value(i);
        set_page(view, group->value(group->order.back()));
    }
    tabBar->setCurrentIndex(tabBar->model()->index (0, 0));
}

void TabWidget::download (QWebEngineDownloadRequest* download)
{
    Q_ASSERT (download && download->state() == QWebEngineDownloadRequest::DownloadRequested);

    // qDebug() << "Download mime:" << download->mimeType();

    if (!download->downloadFileName().contains(".")) // No extension?
    {
        QMimeDatabase db;
        QString suffix = db.mimeTypeForName (download->mimeType()).preferredSuffix();
        if (suffix == "jfif")
        {
            suffix = "jpg";
        }
        download->setDownloadFileName (download->downloadFileName() + "." + suffix);
    }

    QString path = QFileDialog::getSaveFileName (this, tr("Save as"), QDir (download->downloadDirectory()).filePath (download->downloadFileName()));
    if (path.isEmpty())
        return;

    download->setDownloadDirectory (QFileInfo(path).path());
    download->setDownloadFileName (QFileInfo(path).fileName());

    download->accept();
}

SettingsTab* TabWidget::settings_tab()
{
    SettingsTab* view = new SettingsTab (this);
    QStandardItem* item = new QStandardItem (tr("Settings"));
    item->setData (reinterpret_cast<quintptr>(view), Qt::UserRole);
    item->setDropEnabled (false);
    QStandardItemModel* model = static_cast<QStandardItemModel*>(tabBar->model());

    insertWidget (currentIndex()+1, view);
    int row = tabBar->selectionModel()->selectedIndexes().first().row();
    model->insertRow (row+1, item);
    connect(view, &SettingsTab::reload_filters, this, &TabWidget::reload_filters);
    connect(view, &SettingsTab::name_update, this, &TabWidget::name_update);
    connect(view, &SettingsTab::status_update, this, &TabWidget::status_update);
    setTabIcon(tabBar->indexOf(view), QIcon(QStringLiteral (":/icons/system")));
    setCurrentWidget(view);
    tabBar->setCurrentIndex(tabBar->model()->index (tabBar->indexOf (view), 0));
    return view;
}

DebugTab* TabWidget::debug_tab()
{
    DebugTab* view = new DebugTab(this);
    QStandardItem* item = new QStandardItem(tr("Debug"));
    item->setDropEnabled (false);
    item->setData(reinterpret_cast<quintptr>(view), Qt::UserRole);
    QStandardItemModel* model = static_cast<QStandardItemModel*>(tabBar->model());
    insertWidget (currentIndex()+1, view);
    int row = tabBar->selectionModel()->selectedIndexes().first().row();
    model->insertRow (row+1, item);

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
            emit view->redraw_tabs(tab_groups);
            emit view->redraw_history(history);
        }
    });

    setTabIcon(tabBar->indexOf(view), QIcon(QStringLiteral(":/icons/system")));
    setCurrentWidget(view);
    tabBar->setCurrentIndex(tabBar->model()->index(tabBar->indexOf(view), 0));
    emit debug_tabs_updated();

    return view;
}

ProcessTab* TabWidget::show_process_tab()
{
    QStandardItem* item = new QStandardItem(tr("Downloads"));
    item->setData(reinterpret_cast<quintptr>(process_tab), Qt::UserRole);
    item->setDropEnabled(false);
    QStandardItemModel* model = static_cast<QStandardItemModel*>(tabBar->model());

    insertWidget(currentIndex()+1, process_tab);
    int row = tabBar->selectionModel()->selectedIndexes().first().row();
    model->insertRow(row+1, item);
    setTabIcon(tabBar->indexOf(process_tab), QIcon(QStringLiteral(":/icons/download")));
    setCurrentWidget(process_tab);
    tabBar->setCurrentIndex(tabBar->model()->index(tabBar->indexOf(process_tab), 0));
    return process_tab;
}

void TabWidget::fullscreen_request(QWebEngineFullScreenRequest request)
{
    if (auto view = qobject_cast<WebView*>(widget(currentIndex())))
    {
        if (request.toggleOn())
        {
            if (fullscreen)
                return;
            request.accept();
            fullscreen.reset(new FullScreenWindow(view));
        } else
        {
            if (!fullscreen)
                return;
            request.accept();
            fullscreen.reset();
        }
    }
}

WebView* TabWidget::assign_host_view(const QString& host)
{
    if (host_views.contains(host))
    {
        WebView* view = host_views.value(host);
        return view;
    }
    else
    {
        WebView* view = create_tab();
        host_views.insert(host, view);
        return view;
    }
}

TabGroup* TabWidget::assign_tab_group (const QString& host)
{
    if (tab_groups.contains (host))
    {
        TabGroup* group = tab_groups.value(host);
        return group;
    }
    else
    {
        TabGroup* group = new TabGroup;
        group->profile = profile;
        tab_groups.insert(host, group);
        return group;
    }
}

// Ensure the pages are deleted before the profile release.
void TabWidget::cleanup()
{
    save_state();

    for (int row = 0; row < tabBar->model()->rowCount(); ++row)
    {
        if (auto view = qobject_cast<WebView*>(tabBar->widget(row)))
        {
            view->setPage(nullptr);
        }
    }

    for (auto i = tab_groups.begin(); i != tab_groups.end(); ++i)
    {
        TabGroup* group = i.value();
        for (auto j = group->begin(); j != group->end(); ++j)
        {
            j.value().data()->disconnect();
            j.value().clear();
        }
        delete(group);
    }
    qDebug() << "Cleanup complete.";
}

QWeakPointer<WebPage> TabWidget::page_back(TabGroup* group)
{
    for (auto i=history.rbegin(); i!=history.rend(); ++i)
    {
        QString s = i->toString(); //.adjusted (QUrl::RemoveQuery)
        if (group->contains(s))
        {
            qDebug() << "Switching to page" << i->toString();
            history.add(*i);
            return (group->get_page(s));
        }
    }
    return QWeakPointer<WebPage>();
}

QWeakPointer<WebPage> TabWidget::page_forward(TabGroup* group)
{
    for (auto i=history.rbegin(); i!=history.rend(); ++i)
    {
        QString s = i->toString(); //.adjusted (QUrl::RemoveQuery)
        if (group->contains (s))
        {
            qDebug() << "Switching to page" << i->toString();
            history.add (*i);
            return (group->get_page(s));
        }
    }
    return QWeakPointer<WebPage>();
}
