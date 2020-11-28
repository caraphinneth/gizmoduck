#include "tox_ui.h"
#include "file_dialog.h"
#include "message_log.h"

extern Tox_State g_tox_state;

static ToxManager* g_tox_manager;

void friend_message_cb(Tox* /*tox*/, uint32_t friend_number, TOX_MESSAGE_TYPE /*type*/, const uint8_t* message, size_t /*length*/, void* /*user_data*/)
{
    if (g_tox_manager!=nullptr)
        emit g_tox_manager->friend_message_received (QString((const char*)message), friend_number);
}

void friend_name_cb(Tox* /*tox*/, uint32_t friend_number, const uint8_t* name, size_t /*length*/, void* /*user_data*/)
{
    if (g_tox_manager!=nullptr)
        emit g_tox_manager->friend_name_changed (QString((const char*)name), friend_number);
}

void friend_connection_status_cb (Tox* /*tox*/, uint32_t friend_number, TOX_CONNECTION connection_status, void* /*user_data*/)
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
        emit g_tox_manager->friend_connection_status_changed (status, friend_number);
    }
}

void friend_status_cb(Tox* /*tox*/, uint32_t friend_number, TOX_USER_STATUS status, void* /*user_data*/)
{
    if (g_tox_manager!=nullptr)
    {
        QString _status = "unknown";
        switch (status) {
            case TOX_USER_STATUS_NONE:
            _status = "online";
            break;
            case TOX_USER_STATUS_AWAY:
            _status = "away";
            break;
            case TOX_USER_STATUS_BUSY:
            _status = "busy";
            break;
        }
        emit g_tox_manager->friend_status_changed (_status, friend_number);
    }
}

void friend_typing_cb (Tox* /*tox*/, uint32_t friend_number, bool is_typing, void* /*user_data*/)
{
    if (g_tox_manager!=nullptr)
        emit g_tox_manager->friend_typing (is_typing, friend_number);
}

void self_connection_status_cb (Tox* /*tox*/, TOX_CONNECTION connection_status, void* /*user_data*/)
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

void file_receive_cb (Tox* tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t* filename, size_t /*filename_length*/, void* /*user_data*/)
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

                accept = (old_hash != new_hash);
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

void file_chunk_receive_cb (Tox* /*tox*/, uint32_t friend_number, uint32_t file_number, uint64_t /*position*/, const uint8_t* data, size_t length, void* /*user_data*/)
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
        file->deleteLater();
    }
    else
        file->write (reinterpret_cast<const char*>(data), length);
}

void file_chunk_send_cb (Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, __attribute__((unused)) void* user_data)
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

    tox_callback_self_connection_status (tox, self_connection_status_cb);
    tox_callback_friend_connection_status (tox, friend_connection_status_cb);
    tox_callback_friend_status (tox, friend_status_cb);
    tox_callback_friend_message (tox, friend_message_cb);
    tox_callback_friend_name (tox, friend_name_cb);

    tox_callback_friend_typing (tox, friend_typing_cb);

    tox_callback_file_recv (tox, file_receive_cb);
    tox_callback_file_recv_chunk (tox, file_chunk_receive_cb);

    tox_callback_file_chunk_request (tox, file_chunk_send_cb);

    connect (this, &ToxManager::self_connection_status_changed, [this](const QString& status)
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

QList<ToxContact> ToxManager::contact_list()
{
    QList <struct ToxContact> result;
    const size_t friend_list_length = tox_self_get_friend_list_size (tox);
    QVector<uint32_t> friend_id_list (friend_list_length);
    tox_self_get_friend_list (tox, friend_id_list.data());

    for (size_t i : friend_id_list)
    {
        QString status = "offline";
        TOX_CONNECTION status1 = tox_friend_get_connection_status (tox, i, 0);
        if (status1 != TOX_CONNECTION_NONE)
        {
            TOX_USER_STATUS status2 = tox_friend_get_status (tox, i, 0);
            switch (status2) {
                case TOX_USER_STATUS_NONE:
                status = "online";
                break;
                case TOX_USER_STATUS_AWAY:
                status = "away";
                break;
                case TOX_USER_STATUS_BUSY:
                status = "busy";
                break;
            }
        }

        QString name = "(Not connected...)";
        if (status != "offline")
        {
            const size_t length = tox_friend_get_name_size (tox, i, 0);
            QVector<uint8_t> buf (length);
            tox_friend_get_name (tox, i, buf.data(), 0);
            name = QString::fromUtf8 (QByteArray (reinterpret_cast<const char*>(buf.data()), length));
            buf.clear();
        }

        struct ToxContact contact = {i, name, status};
        result.push_back (contact);

    }
    return result;
}

ToxWidget::ToxWidget (QWidget* parent, long friend_number): QWidget (parent)
{
    friend_id = friend_number;

    const size_t length = tox_friend_get_name_size (g_tox_manager->tox, friend_id, 0);
    QVector<uint8_t> buf (length);
    tox_friend_get_name (g_tox_manager->tox, friend_id, buf.data(), 0);

    // Yeah it's THAT bad, buf.data() alone will return trailing garbage.
    friend_name =  QString::fromUtf8 (QByteArray (reinterpret_cast<const char*>(buf.data()), length));
    buf.clear();

    QLabel* label = new QLabel (tr("Chat with ") + friend_name, this);
    QLabel* typing_label = new QLabel ("", this);
    QFont font1 ("Roboto", 8);
    typing_label->setFont (font1);
    typing_label->setFixedHeight (16);

    uint8_t pubkey [TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key (g_tox_manager->tox, friend_id, pubkey, nullptr);

    MessageLog* chat_view = new MessageLog (this, QString (QByteArray ((const char*)pubkey).left(TOX_PUBLIC_KEY_SIZE).toHex().toUpper()));
    // chat_view->setTextInteractionFlags(Qt::TextBrowserInteraction);

    input_box = new InputWidget (this);
    typing = new QTimer (this);
    attach_button = new QPushButton (this);
    attach_button->setIcon (QIcon(":/icons/attachment"));

    connect (attach_button, &QPushButton::clicked, [this, chat_view]()
    {
        QScopedPointer <FileDialog> dialog;
        dialog.reset (new FileDialog (this, tr("Select File"), "", tr("All Files (*.*)")));
        dialog->setAcceptMode (QFileDialog::AcceptOpen);

        //if (mode == QWebEnginePage::FileSelectOpen)
            dialog->setFileMode (QFileDialog::ExistingFile);
        //else if (mode == QWebEnginePage::FileSelectOpenMultiple)
            //dialog->setFileMode (QFileDialog::ExistingFiles);

        if (dialog->exec())
        {
             QString filename = dialog->selectedFiles().first();
             emit file_sent (filename, friend_id);
             chat_view->append (filename, ":/icons/tox_online", QDateTime::currentDateTime(), true);
        }

        dialog->deleteLater();

    });

    connect (input_box, &InputWidget::paste_image, [this, chat_view] (const QPixmap& pixmap)
    {
        // akin to qTox but hopefully faster
        QString filepath = appdata_path % QDir::separator() % QString ("qTox_Image_%1.jpg")
                               .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH-mm-ss.zzz"));
        QFile file (filepath);

        if (file.open(QFile::ReadWrite))
        {
            pixmap.save(&file, "JPG");
            file.close();
            QFileInfo fi (file);
            emit file_sent (fi.filePath(), friend_id);
            chat_view->append (fi.filePath(), ":/icons/tox_online", QDateTime::currentDateTime(), true);
        }
        else
        {
            printf ("Failed to write to a temporary file!\n");
        }

    });

    QFormLayout* form = new QFormLayout (this);
    form->addRow (label);
    form->addRow (chat_view);
    form->addRow (typing_label);
    form->addRow (attach_button, input_box);
    setLayout(form);


    QString friend_avatar_path = appdata_path % QDir::separator() % QString (QByteArray ((const char*)pubkey).left(TOX_PUBLIC_KEY_SIZE).toHex().toUpper()) + ".png";
    QFile file (friend_avatar_path);

    printf ("Looking for %s...", QByteArray ((const char*)pubkey).toHex().toUpper().constData());


    if (!file.open (QIODevice::ReadOnly))
    {
        friend_avatar_path = ":/icons/tox_online";
        friend_avatar.load (friend_avatar_path);
        printf (" not found, using default.\n");
    }
    else
    {
        QByteArray pic = file.readAll();
        file.close ();
        friend_avatar.loadFromData (pic);
        printf (" found, loading.\n");
    }



    connect (this, &ToxWidget::friend_typing, [this, typing_label](bool is_typing)
    {
        if (is_typing)
            typing_label->setText (friend_name + tr(" is typing..."));
        else
            typing_label->clear();
    });
    connect (this, &ToxWidget::message_received, [this, chat_view, friend_avatar_path] (const QString& text)
    {
        chat_view->append (text, friend_avatar_path, QDateTime::currentDateTime());
    });

    connect (this, &ToxWidget::file_received, [this, chat_view, friend_avatar_path] (const QString& filename)
    {
        chat_view->append (filename, friend_avatar_path, QDateTime::currentDateTime(), true);
    });

    connect (input_box, &QLineEdit::textEdited, [this]()
    {
        tox_self_set_typing (g_tox_manager->tox, friend_id, true, 0);
        typing->start (2000);
    });

    connect (typing, &QTimer::timeout, [this]()
    {
        tox_self_set_typing (g_tox_manager->tox, friend_id, false, 0);
    });

    connect (input_box, &QLineEdit::returnPressed, [this, chat_view]()
    {
        if (!input_box->text().isEmpty ())
        {
            emit message_sent (input_box->text(), friend_id);
            chat_view->append (input_box->text(), ":/icons/tox_online", QDateTime::currentDateTime());
            input_box->clear ();
        }
    });
}