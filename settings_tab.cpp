#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QListWidget>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

#include "file_dialog.h"
#include "setting_tab.h"
#include "tox_ui.h"

extern "C" Tox_State g_tox_state;

QString read_comment (const QString& filename)
{
    QString ret;
    QFile file (filename);
    if (file.open (QIODevice::ReadOnly))
    {
       QTextStream in(&file);
       if (!in.atEnd())
       {
           ret = in.readLine();
           int i = ret.indexOf ("#");
           if (i != -1)
               ret = ret.right(ret.length() - i - 1);
           else ret = "";
       }
       file.close();
    }
    return ret;
}

void pic2avatar (const QString& filename)
{
    QImage image (filename);
    if (image.isNull())
        return;

    image = image.scaled (256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QFile file (my_avatar);
    if (file.open (QIODevice::WriteOnly))
    {
        image.save (&file, "PNG");
        file.close();
    }
}

SettingsTab::SettingsTab (QWidget* parent): QWidget (parent)
{
    QSettings settings;

    settings.beginGroup ("Messenger");
    QGroupBox* messenger = new QGroupBox (tr("Messenger"), this);
    QLabel* avatar = new QLabel ("My avatar", this);
    avatar->setPixmap (QPixmap (my_avatar));
    QPushButton* change_avatar = new QPushButton ("Change avatar", this);
    QLabel* tox_id = new QLabel ("My tox ID: " + QString (g_tox_state.id), this);
    tox_id->setTextInteractionFlags (Qt::TextSelectableByMouse);
    QLabel* tox_name_label = new QLabel ("My name: ", this);
    QString n = QString (g_tox_state.name);
    QLineEdit* tox_name = new QLineEdit (settings.value ("name", "Duck").toString(), this);
    QLabel* tox_status_label = new QLabel ("My status: ", this);
    QLineEdit* tox_status = new QLineEdit (settings.value ("status", "Toxing on Gizmoduck").toString(), this);
    settings.endGroup();

    QGridLayout* messenger_layout = new QGridLayout (this);
    messenger_layout->addWidget (avatar, 0, 0, 4, 1);
    messenger_layout->addWidget (change_avatar, 4, 0);
    messenger_layout->addWidget (tox_id, 0, 2);
    messenger_layout->addWidget (tox_name_label, 1, 1);
    messenger_layout->addWidget (tox_name, 1, 2);
    messenger_layout->addWidget (tox_status_label, 2, 1);
    messenger_layout->addWidget (tox_status, 2, 2);
    messenger->setLayout (messenger_layout);

    connect (change_avatar, &QPushButton::clicked, [this, avatar]()
    {
        QScopedPointer <FileDialog> dialog;
        dialog.reset (new FileDialog (this, tr("Select File"), "", tr("All Files (*.*)")));
        dialog->setAcceptMode (QFileDialog::AcceptOpen);
        dialog->setFileMode (QFileDialog::ExistingFile);

        if (dialog->exec())
        {
             QString filename = dialog->selectedFiles().first();
             pic2avatar (filename);
             avatar->setPixmap (QPixmap (my_avatar));
             emit avatar_changed();
        }
    });

    connect (tox_name, &QLineEdit::editingFinished, [this, tox_name]()
    {
        QSettings settings;
        settings.beginGroup ("Messenger");
        settings.setValue ("name", tox_name->text());
        settings.endGroup ();
        emit name_update (tox_name->text());
    });

    connect (tox_status, &QLineEdit::editingFinished, [this, tox_status]()
    {
        QSettings settings;
        settings.beginGroup ("Messenger");
        settings.setValue ("status", tox_status->text());
        settings.endGroup ();
        emit status_update (tox_status->text());
    });

    QGroupBox* content_filters = new QGroupBox (tr("Content Filters:"), this);
    QListWidget* filter_selector = new QListWidget (this);
    QLabel* comment = new QLabel ("Comment", this);

    QStringList all_filters;
    QDir dir ("./whitelist");
    QStringList files = dir.entryList (QDir::Files);
    foreach (const QString& filename, files)
    {
        all_filters.append (filename);
    }

    settings.beginGroup ("Filtering");
    QStringList enabled_filters = settings.value ("enabled").toStringList();
    qDebug() << enabled_filters;
    settings.endGroup();

    foreach (const QString& filename, all_filters)
    {
        QListWidgetItem* item = new QListWidgetItem (filename);
        item->setFlags (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        if (enabled_filters.contains (filename))
            item->setCheckState (Qt::Checked);
        else
            item->setCheckState (Qt::Unchecked);

        filter_selector->addItem (item);
    }

    connect (filter_selector, &QListWidget::itemClicked, [this, filter_selector, comment](QListWidgetItem* item)
    {
        QStringList list;

        for (int i = 0; i < filter_selector->count(); ++i)
        {
            if (filter_selector->item(i)->checkState() == Qt::Checked)
                list.append (filter_selector->item(i)->text());
        }

        QSettings settings;
        settings.beginGroup ("Filtering");
        if (settings.value ("enabled").toStringList() !=list)
        {
            settings.setValue ("enabled", QVariant::fromValue(list));
            emit reload_filters();
        }
        settings.endGroup();
        comment->setText (read_comment ("./whitelist/"+item->text()));
    });


    QVBoxLayout* filtering_layout = new QVBoxLayout (this);
    filtering_layout->addWidget (filter_selector);
    filtering_layout->addWidget (comment);
    content_filters->setLayout (filtering_layout);

    QVBoxLayout* layout = new QVBoxLayout (this);
    layout->addWidget (messenger);
    layout->addWidget (content_filters);
    layout->addStretch (1);
    setLayout(layout);
}
