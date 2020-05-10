#include "tox_ui.h"
#include "file_dialog.h"
#include "message_log.h"

extern Tox_State g_tox_state;

static ToxManager* g_tox_manager;

void friend_message_cb(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message, size_t length, void *user_data)
{
    if (g_tox_manager!=nullptr)
        emit g_tox_manager->friend_message_received (QString((const char*)message), friend_number);
}

void friend_name_cb(Tox *tox, uint32_t friend_number, const uint8_t *name, size_t length, void *user_data)
{
    if (g_tox_manager!=nullptr)
        emit g_tox_manager->friend_name_changed (QString((const char*)name), friend_number);
}

void self_connection_status_cb(Tox *tox, TOX_CONNECTION connection_status, void *user_data)
{
    if (g_tox_manager!=nullptr)
    {
        QString status = "unknown";
        switch (connection_status) {
            case TOX_CONNECTION_NONE:
            status = "offline";
            break;
            case TOX_CONNECTION_TCP:
            status = "online (TCP)";
            break;
            case TOX_CONNECTION_UDP:
            status = "online (UDP)";
            break;
        }
        emit g_tox_manager->self_connection_status_changed (status);
    }
    // printf ("%s\n", g_tox_state.self_online_status);
}

void file_receive_cb (Tox* tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t* filename, size_t filename_length, void* user_data)
{
    if (kind == TOX_FILE_KIND_AVATAR) {
        if (!file_size) {
            printf("Received avatar clear request, stub...\n");
            tox_file_control (tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, nullptr);
            // emit core->friendAvatarRemoved(core->getFriendPublicKey(friendId));
            return;
        } else {
            printf("Receiving avatar update...");

            uint8_t new_hash [TOX_FILE_ID_LENGTH];
            tox_file_get_file_id (tox, friend_number, file_number, new_hash, nullptr);
            // QByteArray avatarBytes {static_cast<const char*>(static_cast<const void*>(avatarHash)), TOX_HASH_LENGTH}; // Borrowed from QTox, but it's just WTF.

            uint8_t pubkey [TOX_PUBLIC_KEY_SIZE];
            tox_friend_get_public_key (tox, friend_number, pubkey, nullptr);

            QFile old_file (appdata_path % QDir::separator() %  QString (QByteArray ((const char*)pubkey).left(TOX_PUBLIC_KEY_SIZE).toHex().toUpper()) + ".png");
            bool accept = false;

            if (!old_file.open (QIODevice::ReadOnly))
            {
                printf(" no old avatar found, accepting.\n");
                 accept = true;
            }
            else
            {
                QByteArray pic = old_file.readAll();
                old_file.close ();
                QByteArray old_hash (TOX_HASH_LENGTH, 0);

                tox_hash (reinterpret_cast<uint8_t*>(old_hash.data()), reinterpret_cast<uint8_t*>(pic.data()), pic.size());

                accept = old_hash != new_hash;
            }

            // Already have this avatar
            if (!accept)
            {
                printf(" already cached, dropping.\n");
                tox_file_control (tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, nullptr);
                return;
            }
            else
            {
                printf(" hash not matching, accepting.\n");
                tox_file_control (tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME, nullptr);
                QFile* file = new QFile (appdata_path % QDir::separator() %  QString (QByteArray ((const char*)pubkey).left(TOX_PUBLIC_KEY_SIZE).toHex().toUpper()) + ".png");
                file->open (QIODevice::ReadWrite);
                g_tox_manager->files_in_transfer.insert (qMakePair(friend_number, file_number), file);
                return;
            }
            return;
        }
    }

    tox_file_control (tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME, nullptr);
    QFile* file = new QFile (download_path % QDir::separator() %  QString ((const char*)filename));
    file->open (QIODevice::ReadWrite);
    g_tox_manager->files_in_transfer.insert (qMakePair(friend_number, file_number), file);
}

void file_chunk_receive_cb (Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t* data, size_t length, void* user_data)
{
    QPair <uint32_t, uint32_t> key (friend_number, file_number);
    QFile* file = g_tox_manager->files_in_transfer.value (key);
    if (file == nullptr)
    {
        printf("File not open!\n");
    }
    else if (!length)
    {
        file->close();
        g_tox_manager->files_in_transfer.remove (key);
        emit g_tox_manager->download_finished (file->fileName(), friend_number);
        file->deleteLater ();
    }
    else
        file->write (reinterpret_cast<const char*>(data), length);
}

void file_chunk_send_cb (Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void* user_data)
{
    QPair <uint32_t, uint32_t> key (friend_number, file_number);
    QFile* file = g_tox_manager->files_in_transfer.value (key);
    if (file == nullptr)
    {
        printf("File not open!\n");
    }
    else if (!length)
    {
        file->close();
        g_tox_manager->files_in_transfer.remove (key);
        //emit g_tox_manager->upload_finished (file->fileName(), friend_number);
        file->deleteLater();
    }
    else
    {
        std::unique_ptr<uint8_t[]> data (new uint8_t[length]);
        file->seek (position);
        tox_file_send_chunk (tox, friend_number, file_number, position, data.get(), file->read((char*)data.get(), length), nullptr);
    }
}

void ToxManager::iterate ()
{
    tox_iterate(tox, 0);
}

void ToxManager::echo (const QString &message, const long friend_number)
{
    QByteArray ba = message.toUtf8();
    // printf ("Attempting to send %s\n", ba.constData());
    tox_friend_send_message(tox, friend_number, TOX_MESSAGE_TYPE_NORMAL, reinterpret_cast<const uint8_t*>(ba.constData()), ba.size(), 0);
}

void ToxManager::send_file (const QString &filename, const long friend_number)
{
    QByteArray ba = QFileInfo(filename).fileName().toUtf8();
    QFile* file = new QFile (filename);
    file->open (QIODevice::ReadOnly);
    uint32_t file_number = tox_file_send (tox, friend_number, TOX_FILE_KIND_DATA, file->size(), nullptr, reinterpret_cast<const uint8_t*>(ba.constData()), ba.size(), 0);
    files_in_transfer.insert (qMakePair (friend_number, file_number), file);
}


ToxManager::ToxManager (): QObject()
{
    printf("Starting Tox...\n");
    tox = create_tox();
    start_tox (tox);

    g_tox_manager = this;

    tox_callback_self_connection_status(tox, self_connection_status_cb);

    tox_callback_friend_message (tox, friend_message_cb);
    tox_callback_friend_name (tox, friend_name_cb);

    tox_callback_file_recv (tox, file_receive_cb);
    tox_callback_file_recv_chunk (tox, file_chunk_receive_cb);

    tox_callback_file_chunk_request (tox, file_chunk_send_cb);

    connect (this, &ToxManager::self_connection_status_changed, [this](const QString &status)
    {
        if (status == "offline")
        {
            watchdog->start (15*1000); // 15 seconds
        }
        else
        {
            watchdog->stop();
        }
        // printf("Status changed to: %s!\n", g_tox_state.self_online_status);
    });

    watchdog = new QTimer (this);
    connect(watchdog, &QTimer::timeout, this, [this]() {
        printf("Tox connection gone cold, reconnecting...\n");
        bootstrap(tox);
    });

    id = QString (g_tox_state.id);
    // self_online_status = QString (g_tox_state.self_online_status);
    iteration_interval = tox_iteration_interval (tox);

    timer = new QTimer (this);
    connect(timer, &QTimer::timeout, this, &ToxManager::iterate);
    timer->start (iteration_interval);

    connect(this, &ToxManager::message_sent, this, &ToxManager::echo);
}

ToxManager::~ToxManager ()
{
    watchdog->stop ();
    timer->stop ();
    printf("Stopping Tox...\n");
    stop_tox(tox);

    g_tox_manager = nullptr;
}

void ToxManager::message (const QString &text, const long friend_number)
{
    emit message_sent (text, friend_number);
}

ToxWidget::ToxWidget (QWidget *parent, long friend_number): GLWidget (parent)
{
    friend_id = friend_number;

    size_t length = tox_friend_get_name_size (g_tox_manager->tox, friend_id, 0);
    QVector<uint8_t> buf (length);
    tox_friend_get_name (g_tox_manager->tox, friend_id, buf.data(), 0);

    // Yeah it's THAT bad, buf.data() alone will return trailing garbage.
    friend_name =  QString::fromUtf8 (QByteArray (reinterpret_cast<const char*>(buf.data()), length));
    buf.clear();

    QLabel *label = new QLabel (tr("Chat with ") + friend_name, this);
    MessageLog *chat_view = new MessageLog (this);
    // chat_view->setTextInteractionFlags(Qt::TextBrowserInteraction);

    input_box = new QLineEdit (this);
    attach_button = new QPushButton (this);
    attach_button->setIcon (QIcon(":/icons/attachment"));

    connect (attach_button, &QPushButton::clicked, [this, chat_view]()
    {
        FileDialog *dialog = new FileDialog (this, tr("Select File"), "", tr("All Files (*.*)"));
        dialog->setAcceptMode (QFileDialog::AcceptOpen);

        //if (mode == QWebEnginePage::FileSelectOpen)
            dialog->setFileMode (QFileDialog::ExistingFile);
        //else if (mode == QWebEnginePage::FileSelectOpenMultiple)
            //dialog->setFileMode (QFileDialog::ExistingFiles);

        if (dialog->exec())
        {
             QString filename = dialog->selectedFiles().first();
             emit file_sent (filename, friend_id);
             chat_view->append (filename, QPixmap (QStringLiteral (":/icons/tox_online")), QDateTime::currentDateTime(), true);
        }

        dialog->deleteLater();

    });

    QFormLayout* form = new QFormLayout (this);
    form->addRow (label);
    form->addRow (chat_view);
    form->addRow (attach_button, input_box);
    setLayout(form);

    uint8_t pubkey [TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key (g_tox_manager->tox, friend_id, pubkey, nullptr);

    QFile file (appdata_path % QDir::separator() % QString (QByteArray ((const char*)pubkey).left(TOX_PUBLIC_KEY_SIZE).toHex().toUpper()) + ".png");

    printf ("Looking for %s...", QByteArray ((const char*)pubkey).toHex().toUpper().constData());

    if (!file.open (QIODevice::ReadOnly))
    {
        friend_avatar.load (QStringLiteral (":/icons/tox_online"));
        printf (" not found, using default.\n");
    }
    else
    {
        QByteArray pic = file.readAll();
        file.close ();
        friend_avatar.loadFromData (pic);
        printf (" found, loading.\n");
    }

    connect (this, &ToxWidget::message_received, [this, chat_view] (const QString &text)
    {
        chat_view->append (text, friend_avatar, QDateTime::currentDateTime());
    });

    connect (this, &ToxWidget::file_received, [this, chat_view] (const QString &filename)
    {
        chat_view->append (filename, friend_avatar, QDateTime::currentDateTime(), true);
    });

    connect (input_box, &QLineEdit::returnPressed, [this, chat_view]()
    {
        if (!input_box->text().isEmpty ())
        {
            emit message_sent (input_box->text(), friend_id);
            chat_view->append (input_box->text(), QPixmap (QStringLiteral (":/icons/tox_online")), QDateTime::currentDateTime());
            input_box->clear ();
        }
    });
}