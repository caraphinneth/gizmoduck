#include <QNetworkProxyFactory>
#include <QSettings>
#include <QToolBar>
#include <QTabBar>
#include <QVBoxLayout>

#include "browser_mainwindow.h"
#include "dockwidget.h"
#include "tab_manager.h"

// On resize events, reapply the expanding tabs style sheet (disabled for now).
class ResizeFilter : public QObject
{
    QTabWidget *target;
    public:
    ResizeFilter (QTabWidget *target) : QObject(target), target(target) {}

    bool eventFilter (QObject, QEvent *event)
    {
        if (event->type() == QEvent::Resize)
        {
            // The width of each tab is the width of the tab widget / # of tabs.
            target->setStyleSheet (QString ("QTabBar::tab { width: %1px; } ") .arg(target->size().width()/target->count()-1));
        }
        return false;
    }
};

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

    QToolBar *toolbar = new QToolBar (tr("Navigation"), this);
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

    tox_button = new NavButton (toolbar);
    tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_connecting")));
    toolbar->addWidget (tox_button);

    toolbar->addWidget (settings_button);

    close_button = new NavButton (toolbar);
    close_button->setIcon (QIcon (QStringLiteral (":/icons/close")));
    close_button->setToolTip(tr("Close the active tab"));
    toolbar->addWidget (close_button);

    addToolBar (toolbar);

    //QWidget *central_widget = new QWidget (this);
    GLWidget *central_widget = new GLWidget (this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing (0);
    layout->setMargin (0);
    addToolBarBreak();

    tab_manager = new TabWidget (this);

    //tab_manager->setCornerWidget(close_button, Qt::BottomLeftCorner);
    //installEventFilter (new ResizeFilter (tab_manager));
    layout->addWidget (tab_manager);
    central_widget->setLayout (layout);
    setCentralWidget (central_widget);

    search_bar = new QToolBar (central_widget);
    layout->addWidget (search_bar);
    QLineEdit *search_box = new QLineEdit (search_bar);
    search_bar->addWidget (search_box);
    search_bar->hide();

    //addToolBar (search_bar);
    addToolBarBreak();

    address_box->setClearButtonEnabled (true);
    //address_box->setFrame(false);
    //address_box->setPlaceholderText (tr("Awaiting input..."));
    QPalette palette;
    palette.setColor (QPalette::Base, QApplication::palette ("QWidget").color (QPalette::Window));
    //palette.setColor (QPalette::Text,Qt::white);
    address_box->setPalette (palette);

    QCompleter *completer = new QCompleter (this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(&tab_manager->model);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    connect (&tab_manager->model, &QStandardItemModel::rowsInserted, [this, proxyModel]()
    {
        proxyModel->sort (0);
    });
    completer->setModel (proxyModel);
    // completer->setCompletionMode (QCompleter::UnfilteredPopupCompletion);
    completer->setModelSorting (QCompleter::CaseInsensitivelySortedModel);
    completer->setFilterMode (Qt::MatchContains); //Qt::MatchRecursive
    completer->setCaseSensitivity (Qt::CaseInsensitive);
    completer->setMaxVisibleItems (10);

    // Adopt these as classes later.
    QTableView *popup = new QTableView (address_box);
    popup->setAlternatingRowColors (true);
    popup->setMouseTracking (true);
    popup->setShowGrid (false);
    popup->horizontalHeader()->setSectionResizeMode (QHeaderView::Stretch);
    popup->horizontalHeader()->setVisible(false);
    popup->setSelectionBehavior (QAbstractItemView::SelectRows);
    popup->setSelectionMode (QAbstractItemView::SingleSelection);
    //popup->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    //popup->verticalHeader()->setSectionResizeMode (QHeaderView::ResizeToContents);
    popup->verticalHeader()->setVisible(false);
    popup->setItemDelegateForColumn (0, new QStyledItemDelegate (completer));
    popup->setSortingEnabled (false);

    completer->setPopup (popup);
    address_box->setCompleter (completer);

    connect (address_box, &QLineEdit::returnPressed, [this]()
    {
        QUrl url = QUrl::fromUserInput (address_box->text());

        if (url.isValid()&&((url.host()=="localhost")||(!url.topLevelDomain().isEmpty())||(url.topLevelDomain()=="i2p")|| (url.scheme()=="chrome")  ))
        //if (url.isValid() && !url.scheme().isEmpty() && (!url.host().isEmpty() || !url.path().isEmpty() || url.hasQuery()))
            tab_manager->set_url (url);
        else
            tab_manager->set_url (QUrl::fromUserInput ("https://duckduckgo.com/?q="+address_box->text()));
    });

    connect (tab_manager, &TabWidget::url_changed, [this](const QUrl &url)
    {
        WebView *view = qobject_cast<WebView*>(tab_manager->widget (tab_manager->currentIndex()));
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


    connect (back_button, &NavButton::left_clicked, tab_manager, &TabWidget::back);
    connect (forward_button, &NavButton::left_clicked, tab_manager, &TabWidget::forward);

    connect (back_button, &NavButton::mid_clicked, tab_manager, &TabWidget::back_in_new_tab);
    connect (forward_button, &NavButton::mid_clicked, tab_manager, &TabWidget::forward_in_new_tab);

    connect (refresh_button, &NavButton::left_clicked, tab_manager, &TabWidget::refresh);
    connect (refresh_button, &NavButton::right_clicked, tab_manager, &TabWidget::refresh_no_cache);
    refresh_button->setShortcut(tr("F5"));

    connect (settings_button, &NavButton::left_clicked, tab_manager, &TabWidget::settings_tab);
    connect (settings_button, &NavButton::right_clicked, tab_manager, &TabWidget::debug_tab);

    connect (close_button, &NavButton::left_clicked, [this]()
    {
        tab_manager->close_tab (tab_manager->currentIndex());
    });

    connect (close_button, &NavButton::right_clicked, tab_manager, &TabWidget::restore_tab);

    connect (tox_button, &NavButton::left_clicked, [this]()
    {
        printf ("Placeholder!\n");
    });

    tox_manager = new ToxManager;

    connect (tox_manager, &ToxManager::friend_message_received, this, &MainWindow::chat);

    connect (tox_manager, &ToxManager::friend_name_changed, [this](const QString &name, const long friend_number)
    {
        if (active_chats.contains (friend_number))
        {
            active_chats.value (friend_number)->friend_name = name;
            QByteArray ba = name.toUtf8();
            printf ("Changed friend name to %s\n", ba.constData());
        }
    });
    connect (tox_manager, &ToxManager::self_connection_status_changed, [this](const QString &status)
    {
        if (status == "offline")
            tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_connecting")));
        else if (status == "online (TCP)")
            tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_idle")));
        else if (status == "online (UDP)")
            tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_idle")));
        else
            tox_button->setIcon (QIcon (QStringLiteral (":/icons/tox_connecting")));

        tox_button->setToolTip ("Tox status: "+status);
    });
    connect (tox_manager, &ToxManager::download_finished, [this] (const QString &filename, const long friend_number)
    {
        if (active_chats.contains (friend_number))
        {
            ToxWidget *chat_area = active_chats.value (friend_number);
            emit chat_area->file_received (filename);
        }
    });


    QAction *toggle_search = new QAction (this);
    toggle_search->setShortcut (Qt::Key_F | Qt::CTRL);
    connect (toggle_search, &QAction::triggered, [this, search_box]()
    {
        search_bar->setVisible (!search_bar->isVisible());

        if (!search_bar->isVisible())
        {
            WebView *view = qobject_cast<WebView*>(tab_manager->currentWidget());
            if (view)
            {
                view->findText ("");
            }
        }
        else
            search_box->setFocus();
    });
    this->addAction(toggle_search);

    QAction *find_text = new QAction (search_bar);
    find_text->setShortcut (Qt::Key_F3);
    connect (find_text, &QAction::triggered, [this, search_box]()
    {
        WebView *view = qobject_cast<WebView*>(tab_manager->currentWidget());
        if (view)
        {
            view->findText (search_box->text());
        }

    });
    search_bar->addAction (find_text);

    connect (search_box, &QLineEdit::textChanged, [this, search_box]()
    {
        WebView *view = qobject_cast<WebView*>(tab_manager->currentWidget());
        if (view)
        {
            view->findText (search_box->text());
        }
    });


}

TabWidget *MainWindow::tabWidget() const
{
    return tab_manager;
}

void MainWindow::closeEvent (QCloseEvent*)
{
    delete tox_manager;
    save_settings();
}

void MainWindow::chat(const QString &message, const long friend_number)
{
    if (!active_chats.contains (friend_number))
    {
        DockWidget *dock = new DockWidget(tr("Chat"), this);
        dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |  Qt::BottomDockWidgetArea);
        addDockWidget(Qt::RightDockWidgetArea, dock);

        connect (dock, &DockWidget::closed , [this, friend_number]()
        {
            active_chats.remove (friend_number);
        });

        ToxWidget *chat_area = new ToxWidget (dock, friend_number);
        connect (chat_area, &ToxWidget::message_sent, tox_manager, &ToxManager::message);
        connect (chat_area, &ToxWidget::file_sent, tox_manager, &ToxManager::send_file);

        dock->setWidget (chat_area);
        active_chats.insert (friend_number, chat_area);

    }
    ToxWidget *chat_area = active_chats.value (friend_number);
    emit chat_area->message_received (message);
}