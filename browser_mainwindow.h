#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include "tab_manager.h"
#include "navigation_button.h"
#include "tox_ui.h"

struct MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    TabWidget *tabWidget() const; // Returns tab_manager, needed fot handling new window commands from webviews.

protected:
    void closeEvent (QCloseEvent*) override;


private slots:
    void chat(const QString &message, const long friend_number);

private:

    ToxManager *tox_manager;
    TabWidget *tab_manager;

    QToolBar *search_bar;
    QLineEdit *address_box;
    NavButton *back_button;
    NavButton *forward_button;
    NavButton *refresh_button;
    NavButton *close_button;
    NavButton *tox_button;
    NavButton *settings_button;

    QHash <uint32_t, ToxWidget*> active_chats;

    void save_settings();
};
