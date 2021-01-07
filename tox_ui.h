#pragma once

#include <QHash>
#include <QPair>
#include <QTimer>
#include "input_widget.h"
#include "gl_widget.h"
#include "tox_client.h"

const QString appdata_path = QStandardPaths::writableLocation (QStandardPaths::AppDataLocation)+"/Gizmoduck";
const QString download_path = QStandardPaths::writableLocation (QStandardPaths::DownloadLocation);

const QString my_avatar = appdata_path +"/avatar.png";

struct ToxContact
{
    QString name;
    QString connection_status;
    QString user_status;
};

class ToxManager : public QObject
{
    Q_OBJECT

private:
    QTimer* timer;
    QTimer* watchdog;
    int iteration_interval;

public:
    ToxManager();
    ~ToxManager();

    Tox* tox;
    QString id;
    QString self_online_status;

    QMap <quint32, ToxContact> contact_list;

    QHash <QPair <uint32_t, uint32_t>, QFile*> files_in_transfer;

public slots:
    void iterate();
    void message (const QString& text, const long friend_number);
    void echo (const QString& message, const long friend_number);
    void send_file (const QString& filename, const long friend_number);
    void send_avatar (const long friend_number);
    void broadcast_avatar();
    void add_friend (const QString& id);
    void name_update (const QString& name);
    void status_update (const QString& status);

signals:
    void start();
    void stop();
    void friend_message_received (const QString& message, const long friend_number);
    void friend_name_changed (const QString& name, const long friend_number);
    void friend_status_changed (const QString& status, const long friend_number);
    void friend_connection_status_changed (const QString& status, const long friend_number);
    void self_connection_status_changed (const QString& status);
    void message_sent (const QString& message, const long friend_number);
    void friend_typing (const bool is_typing, const long friend_number);

    void download_finished (const QString& filename, const long friend_number);
};

struct ToxWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ToxWidget (QWidget* parent = nullptr, long friend_number = 0);

    InputWidget* input_box;
    QPlainTextEdit* chat_view;
    QPushButton* attach_button;
    QTimer* typing;
    QLabel* typing_label;

    long friend_id;
    QString friend_name;
    QPixmap friend_avatar;

public slots:

signals:
    void message_received (const QString& text);
    void message_sent (const QString& text, const long friend_number);
    void file_sent (const QString& filename, const long friend_number);
    void friend_typing (const bool is_typing);
    void file_received (const QString& filename);
};