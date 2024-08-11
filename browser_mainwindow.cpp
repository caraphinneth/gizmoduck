#include <QApplication>
#include <QCompleter>
#include <QDockWidget>
#include <QGridLayout>
#include <QHeaderView>
#include <QHostInfo>
#include <QMenu>
#include <QNetworkProxyFactory>
#include <QSaveFile>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSqlDatabase>
#include <QTableView>
#include <QToolBar>

#include "browser_mainwindow.h"
#include "tab_manager.h"

void MainWindow::save_settings()
{
    QSettings settings;

    settings.beginGroup ("MainWindow");
    settings.setValue ("size", size());
    settings.setValue ("pos", pos());
    settings.endGroup();
}

MainWindow::MainWindow()
{
    QDir dir;
    dir.mkpath (appdata_path);
    QSettings settings;

    settings.beginGroup ("MainWindow");
    resize (settings.value ("size", QSize (1024, 768)).toSize());
    move (settings.value ("pos", QPoint (200, 200)).toPoint());
    settings.endGroup();

    settings.beginGroup ("Interface");
    QApplication::setWheelScrollLines (settings.value ("scroll_speed", 3).toInt());
    settings.endGroup ();

    QNetworkProxyFactory::setUseSystemConfiguration (true);
//    QNetworkProxyFactory factory;
//    QNetworkProxyFactory::setApplicationProxyFactory (factory);

    QToolBar* toolbar = new QToolBar (tr("Navigation"), this);
    toolbar->setContextMenuPolicy (Qt::CustomContextMenu);
    //toolbar->setMovable(false);
    //toolbar->toggleViewAction()->setEnabled(true);

    back_button = new NavButton (toolbar);
    back_button->setIcon (QIcon (QStringLiteral (":/icons/back")));
    forward_button = new NavButton (toolbar);
    forward_button->setIcon (QIcon (QStringLiteral (":/icons/forward")));
    refresh_button = new NavButton (toolbar);
    refresh_button->setIcon (QIcon (QStringLiteral (":/icons/refresh")));
    toolbar->addWidget (back_button);
    toolbar->addWidget (forward_button);

    address_box = new QLineEdit (toolbar);
    toolbar->addWidget (address_box);

    toolbar->addWidget (refresh_button);

    settings_button = new NavButton (toolbar);
    settings_button->setIcon (QIcon (QStringLiteral (":/icons/system")));
    settings_button->setToolTip (tr("Settings"));

    contact_menu = new QMenu();

    tox_button = new NavButton (toolbar);
    tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_connecting")));
    tox_button->setMenu (contact_menu);
    tox_button->setEnabled (false);
    toolbar->addWidget (tox_button);

    toolbar->addWidget (settings_button);

    close_button = new NavButton (toolbar);
    close_button->setIcon (QIcon (QStringLiteral (":/icons/close")));
    close_button->setToolTip(tr("Close the active tab"));
    toolbar->addWidget (close_button);

    addToolBar (toolbar);

    QWidget *central_widget = new QWidget (this);
    //GLWidget *central_widget = new GLWidget (this);
    QGridLayout* layout = new QGridLayout;
    layout->setSpacing (0);
    // layout->setMargin (0);
    layout->setContentsMargins(0, 0, 0, 0);
    addToolBarBreak();

    tab_manager = new TabWidget (this);

    //tab_manager->setCornerWidget(close_button, Qt::BottomLeftCorner);
    //installEventFilter (new ResizeFilter (tab_manager));
    layout->addWidget (tab_manager, 0, 0);
    layout->addWidget (tab_manager->tabBar, 0, 1);
    central_widget->setLayout (layout);
    setCentralWidget (central_widget);

    search_bar = new QToolBar (central_widget);
    layout->addWidget (search_bar, 1, 0, 1, -1);
    QLabel* search_label = new QLabel (tr("Find text: "), this);
    search_box = new QLineEdit (search_bar);
    search_bar->addWidget (search_label);
    search_bar->addWidget (search_box);
    search_bar->hide();

    address_box->setClearButtonEnabled (true);
    //address_box->setFrame(false);
    //address_box->setPlaceholderText (tr("Awaiting input..."));
    QPalette palette;
    palette.setColor (QPalette::Base, QApplication::palette ("QWidget").color (QPalette::Window));
    //palette.setColor (QPalette::Text,Qt::white);
    address_box->setPalette (palette);

    QCompleter* completer = new QCompleter (this);
    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel (this);
    proxyModel->setSourceModel (&tab_manager->suggestions);
    proxyModel->setSortCaseSensitivity (Qt::CaseInsensitive);
    connect (&tab_manager->suggestions, &QStandardItemModel::rowsInserted, [proxyModel]()
    {
        proxyModel->sort (0);
    });
    completer->setModel (proxyModel);
    //completer->setCompletionMode (QCompleter::UnfilteredPopupCompletion);
    completer->setModelSorting (QCompleter::CaseInsensitivelySortedModel);
    completer->setFilterMode (Qt::MatchContains); //Qt::MatchRecursive
    completer->setCaseSensitivity (Qt::CaseInsensitive);
    completer->setMaxVisibleItems (10);

    // Adopt these as classes later.
    QTableView* popup = new QTableView (address_box);
    popup->setAlternatingRowColors (true);
    popup->setMouseTracking (true);
    popup->setShowGrid (false);
    popup->horizontalHeader()->setSectionResizeMode (QHeaderView::Stretch);
    popup->horizontalHeader()->setVisible (false);
    popup->setSelectionBehavior (QAbstractItemView::SelectRows);
    popup->setSelectionMode (QAbstractItemView::SingleSelection);
    //popup->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    //popup->verticalHeader()->setSectionResizeMode (QHeaderView::ResizeToContents);
    popup->verticalHeader()->setVisible (false);
    popup->setItemDelegateForColumn (0, new QStyledItemDelegate (completer));
    popup->setSortingEnabled (false);

    completer->setPopup (popup);
    address_box->setCompleter (completer);

    connect (address_box, &QLineEdit::returnPressed, [this]()
    {
        QUrl url = QUrl::fromUserInput (address_box->text());
        QString host = url.host();
        QHostInfo::lookupHost(host, [this, url](const QHostInfo& hostInfo)
        {
            if ((hostInfo.error() == QHostInfo::NoError) || (url.scheme()=="chrome"))
            {
                tab_manager->set_url (url, false);
            }
            else
            {
                tab_manager->set_url (QUrl::fromUserInput ("https://duckduckgo.com/?q="+address_box->text()), false);
            }
        });
    });

    connect (tab_manager, &TabWidget::url_changed, [this](const QUrl& url)
    {
        WebView* view = qobject_cast<WebView*>(tab_manager->widget (tab_manager->currentIndex()));
        if (view)
        {
            address_box->setEnabled(true);
            if (url.toString()!="about:blank")
                address_box->setText (url.toDisplayString());
            else
            {
                address_box->setText("");
                address_box->setFocus();
            }
        }
        else // Not a web tab.
        {
            address_box->setDisabled(true);
            address_box->setText ("");
        }
    });

    connect (tab_manager->tabBar, &SideTabs::doubleClicked, [this]()
    {
        address_box->selectAll();
        address_box->setFocus();
    });

    connect (back_button, &NavButton::left_clicked, tab_manager, &TabWidget::back);
    connect (forward_button, &NavButton::left_clicked, tab_manager, &TabWidget::forward);

    connect (back_button, &NavButton::mid_clicked, tab_manager, &TabWidget::back_in_new_tab);
    connect (forward_button, &NavButton::mid_clicked, tab_manager, &TabWidget::forward_in_new_tab);

    connect (refresh_button, &NavButton::left_clicked, tab_manager, &TabWidget::refresh);
    connect (refresh_button, &NavButton::right_clicked, tab_manager, &TabWidget::refresh_no_cache);

    QAction* refresh = new QAction (this);
    refresh->setShortcut (Qt::Key_F5);
    connect (refresh, &QAction::triggered, tab_manager, &TabWidget::refresh);
    this->addAction (refresh);

    connect (settings_button, &NavButton::left_clicked, tab_manager, &TabWidget::settings_tab);
    connect (settings_button, &NavButton::right_clicked, tab_manager, &TabWidget::debug_tab);

    connect (close_button, &NavButton::left_clicked, [this]()
    {
        tab_manager->close_page (tab_manager->tabBar->currentIndex().row());
    });

    connect (close_button, &NavButton::right_clicked, tab_manager, &TabWidget::restore_tab);

    tox_manager = new ToxManager;

    load_contacts();

    connect (tox_button, &NavButton::left_clicked, [this]()
    {
        contact_menu->clear();
        QMap<quint32, ToxContact>::const_iterator i = tox_manager->contact_list.constBegin();
        while (i != tox_manager->contact_list.constEnd())
        {
            QString name = (i.value().name.isEmpty()) ? tr("[Connecting...]") : i.value().name;
            //QString status = (i.value().status.isEmpty()) ? tr("offline") : i.value().status;
            QString status;
            if ((i.value().connection_status == "offline")||i.value().connection_status.isEmpty())
            {
                status = ":/icons/tox_offline";
            }
            else if ((i.value().user_status == "unknown")||i.value().user_status.isEmpty())
            {
                status = ":/icons/tox_connecting";
            }
            else if (i.value().user_status == "away")
            {
                status = ":/icons/tox_idle";
            }
            else if (i.value().user_status == "busy")
            {
                status = ":/icons/tox_busy";
            }
            else if (i.value().user_status == "online")
            {
                status = ":/icons/tox_online";
            }

            QAction* action = new QAction (QIcon (status), name, this);
            const quint32 number = i.key();
            connect (action, &QAction::triggered, [this, number]()
            {
                chat ("", number);
            });
            contact_menu->addAction (action);
            ++i;
        }
        const size_t friend_list_length = tox_self_get_friend_list_size (tox_manager->tox);
        if (friend_list_length > tox_manager->contact_list.size())
        {
            QAction* action = new QAction (tr("Fetching contacts..."), this);
            contact_menu->addAction (action);
        }
        QAction* action = new QAction (tr("Add contact..."), this);
        connect (action, &QAction::triggered, [this]()
        {
            add_contact();
        });
        contact_menu->addAction (action);
    });

    connect (tab_manager, &TabWidget::avatar_changed, tox_manager, &ToxManager::broadcast_avatar);
    connect (tab_manager, &TabWidget::name_update, tox_manager, &ToxManager::name_update);
    connect (tab_manager, &TabWidget::status_update, tox_manager, &ToxManager::status_update);

    connect (tox_manager, &ToxManager::friend_message_received, this, &MainWindow::chat);

    connect (tox_manager, &ToxManager::friend_typing, [this](bool is_typing, const long friend_number)
    {
        if (active_chats.contains (friend_number))
        {
            emit active_chats.value (friend_number)->friend_typing (is_typing);
        }
    });

    connect (tox_manager, &ToxManager::friend_name_changed, [this](const QString& name, const long friend_number)
    {
        ToxContact contact = tox_manager->contact_list.value (friend_number);
        contact.name = name;
        tox_manager->contact_list.insert (friend_number, contact);

        if (active_chats.contains (friend_number))
        {
            active_chats.value (friend_number)->friend_name = name;
            QByteArray ba = name.toUtf8();
            printf ("Changed friend name to %s\n", ba.constData());
        }
    });

    connect (tox_manager, &ToxManager::friend_connection_status_changed, [this](const QString& status, const long friend_number)
    {
        ToxContact contact = tox_manager->contact_list.value (friend_number);
        contact.connection_status = status;
        tox_manager->contact_list.insert (friend_number, contact);
    });

    connect (tox_manager, &ToxManager::friend_status_changed, [this](const QString& status, const long friend_number)
    {
        ToxContact contact = tox_manager->contact_list.value (friend_number);
        contact.user_status = status;
        tox_manager->contact_list.insert (friend_number, contact);
    });

    connect (tox_manager, &ToxManager::self_connection_status_changed, [this](const QString& status)
    {
        if (status == "offline")
        {
            tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_connecting")));
            tox_button->setEnabled (false);
        }

        else if ((status == "online (TCP)")||(status == "online (UDP)"))
        {
            tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_idle")));
            tox_button->setEnabled (true);
        }
        else
        {
            tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_connecting")));
            tox_button->setEnabled (false);
        }

        tox_button->setToolTip ("Tox status: "+status);
    });

    connect (tox_manager, &ToxManager::download_finished, [this] (const QString& filename, const long friend_number)
    {
        // Q&D: technical files should not be displayed.
        if (!filename.contains (appdata_path))
        {
        // FIXME: duplicated code.
        if (!active_chats.contains (friend_number))
        {
            QDockWidget* dock = new QDockWidget (tr("Chat"), this);
            dock->setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
            addDockWidget (Qt::RightDockWidgetArea, dock);

            ToxWidget* chat_area = new ToxWidget (dock, friend_number);
            connect (chat_area, &ToxWidget::message_sent, tox_manager, &ToxManager::message);
            connect (chat_area, &ToxWidget::file_sent, tox_manager, &ToxManager::send_file);

            dock->setWidget (chat_area);
            active_chats.insert (friend_number, chat_area);
        }

        {
            ToxWidget* chat_area = active_chats.value (friend_number);
            QDockWidget* dock = qobject_cast<QDockWidget*>(chat_area->parentWidget());
            dock->show();
            emit chat_area->file_received (filename);
        }
        }
    });

    QAction* toggle_search = new QAction (this);
    toggle_search->setShortcut (Qt::Key_F | Qt::CTRL);
    connect (toggle_search, &QAction::triggered, [this]()
    {
        search_bar->setVisible (!search_bar->isVisible());

        if (!search_bar->isVisible())
        {
            WebView* view = qobject_cast<WebView*>(tab_manager->currentWidget());
            if (view)
            {
                view->findText ("");
            }
        }
        else
            search_box->setFocus();
    });
    this->addAction(toggle_search);

    QAction* find_text = new QAction (search_bar);
    find_text->setShortcut (Qt::Key_F3);
    find_text->setIcon (QIcon (QStringLiteral (":/icons/down")));
    connect (find_text, &QAction::triggered, [this]()
    {
        WebView* view = qobject_cast<WebView*>(tab_manager->currentWidget());
        if (view)
        {
            view->findText (search_box->text());
        }

    });
    search_bar->addAction (find_text);

    QAction* reverse_find_text = new QAction (search_bar);
    //reverse_find_text->setShortcut (Qt::Key_F3);
    reverse_find_text->setIcon (QIcon (QStringLiteral (":/icons/up")));
    connect (reverse_find_text, &QAction::triggered, [this]()
    {
        WebView* view = qobject_cast<WebView*>(tab_manager->currentWidget());
        if (view)
        {
            view->findText (search_box->text(), QWebEnginePage::FindBackward);
        }

    });
    search_bar->addAction (reverse_find_text);


    connect (search_box, &QLineEdit::textChanged, [this]()
    {
        WebView* view = qobject_cast<WebView*>(tab_manager->currentWidget());
        if (view)
        {
            view->findText (search_box->text());
        }
    });

    QSqlDatabase db = QSqlDatabase::addDatabase ("QSQLITE");
    db.setDatabaseName ("testing.db");
    if (!db.open())
    {
        qDebug() << "Unable to create an sqlite connection!";
        exit (EXIT_FAILURE);
    }
}

/*
TabWidget* MainWindow::tabWidget() const
{
    return tab_manager;
}*/

void MainWindow::closeEvent (QCloseEvent* event)
{
    save_contacts();
    delete tox_manager;

    tab_manager->cleanup();
    tab_manager->deleteLater();
    save_settings();
    QMainWindow::closeEvent(event);
}

void MainWindow::open_url(const QUrl url)
{
    tab_manager->set_url(url);
    raise();
    activateWindow();
}

void MainWindow::chat (const QString &message, const long friend_number)
{
    if (!active_chats.contains (friend_number))
    {
        QDockWidget* dock = new QDockWidget (tr("Chat"), this);
        dock->setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        addDockWidget (Qt::RightDockWidgetArea, dock);

        ToxWidget* chat_area = new ToxWidget (dock, friend_number);
        connect (chat_area, &ToxWidget::message_sent, tox_manager, &ToxManager::message);
        connect (chat_area, &ToxWidget::file_sent, tox_manager, &ToxManager::send_file);

        dock->setWidget (chat_area);
        active_chats.insert (friend_number, chat_area);
    }
    ToxWidget* chat_area = active_chats.value (friend_number);
    QDockWidget* dock = qobject_cast<QDockWidget*>(chat_area->parentWidget());
    dock->show();
    if (!message.isEmpty())
    {
        emit chat_area->message_received (message);
    }
}

void MainWindow::save_contacts()
{
    QSaveFile file ("contacts.dat");
    file.open (QIODevice::WriteOnly);
    QDataStream out (&file);
    QMap<quint32, ToxContact>::const_iterator i = tox_manager->contact_list.constBegin();
    while (i != tox_manager->contact_list.constEnd())
    {
        out << i.key();
        out << i.value().name;
        ++i;
    }
    file.commit();
}

void MainWindow::load_contacts()
{
    QFile file ("contacts.dat");
    file.open (QIODevice::ReadOnly);
    QDataStream in (&file);
    while (!in.atEnd())
    {
        quint32 number;
        QString name;
        in >> number;
        in >> name;
        ToxContact contact;
        contact.name = name;
        contact.connection_status = "offline";
        contact.user_status = "unknown";

        tox_manager->contact_list.insert (number, contact);
    }
    file.close();
}

void MainWindow::add_contact()
{
    QDialog dialog (this);
    dialog.setModal (true);
    dialog.setWindowFlags (dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QScopedPointer <QLabel> label;
    label.reset (new QLabel (tr ("Paste a full tox ID (NOT just public key) to add contact:")));

    QScopedPointer<QLineEdit> pubkey;
    pubkey.reset (new QLineEdit (&dialog));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget (label.data());
    layout->addWidget (pubkey.data());
    dialog.setLayout (layout);

    connect (pubkey.data(), &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted)
    {
        emit tox_manager->add_friend (pubkey->text());
    }
}
