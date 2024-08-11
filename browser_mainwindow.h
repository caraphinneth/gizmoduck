#pragma once
#include <QLineEdit>
#include <QMainWindow>

#include "navigation_button.h"
#include "tab_manager.h"
#include "tox_ui.h"

struct MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    void open_url(const QUrl url);
    TabWidget* tab_manager;

protected:
    void closeEvent (QCloseEvent*) override;

private slots:
    // Handle incoming and outgoing chat messages, creating widgets as needed.
    void chat (const QString& message, const long friend_number);

private:

    ToxManager* tox_manager;

    QToolBar* search_bar;
    QLineEdit* address_box;
    NavButton* back_button;
    NavButton* forward_button;
    NavButton* refresh_button;
    NavButton* close_button;
    NavButton* tox_button;
    NavButton* settings_button;
    QLineEdit* search_box;
    QMenu* contact_menu;

    QHash <uint32_t, ToxWidget*> active_chats;

    void save_settings();
    void save_contacts();
    void load_contacts();
    void add_contact();
};
