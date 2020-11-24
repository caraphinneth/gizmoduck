#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include "tab_manager.h"
#include "navigation_button.h"

#ifdef ENABLE_TOX
    #include "tox_ui.h"
#endif

struct MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    TabWidget* tabWidget() const; // Returns tab_manager, needed for handling new window commands from webviews.

protected:
    void closeEvent (QCloseEvent*) override;

#ifdef ENABLE_TOX
private slots:
    void chat (const QString& message, const long friend_number);
#endif

private:

    ToxManager* tox_manager;
    TabWidget* tab_manager;

    QToolBar* search_bar;
    QLineEdit* address_box;
    NavButton* back_button;
    NavButton* forward_button;
    NavButton* refresh_button;
    NavButton* close_button;
    NavButton* tox_button;
    NavButton* settings_button;

#ifdef ENABLE_TOX
    QHash <uint32_t, ToxWidget*> active_chats;
#endif

    void save_settings();
};
