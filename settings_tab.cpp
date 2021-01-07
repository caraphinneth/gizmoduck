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

    QGroupBox* visual = new QGroupBox (tr("Interface"), this);
    QLabel* scroll_speed_label = new QLabel (tr("Mouse wheel scroll speed:"), this);
    QSpinBox* scroll_speed = new QSpinBox (this);

    settings.beginGroup ("Interface");

    scroll_speed->setValue (settings.value ("scroll_speed", 3).toInt());
    scroll_speed->setRange (1, 99);

    settings.endGroup();

    connect (scroll_speed, QOverload<int>::of (&QSpinBox::valueChanged), [this] (int value)
    {
        QSettings settings;
        settings.beginGroup ("Interface");
        settings.setValue ("scroll_speed", value);
        settings.endGroup ();
        QApplication::setWheelScrollLines (value);
    });

    QVBoxLayout *interface_layout = new QVBoxLayout (this);
    interface_layout->addWidget (scroll_speed_label);
    interface_layout->addWidget (scroll_speed);
    visual->setLayout (interface_layout);

    QGroupBox* mp = new QGroupBox (tr("Process model (applied on restart)"), this);
    QRadioButton* site_per_process = new QRadioButton(tr("Isolated processes. Recommended for security."), this);
    QRadioButton* process_per_site = new QRadioButton(tr("Process per site. This way, tabs of the same origin will share the same process. Recommended for memory conservation."), this);
    QRadioButton* single_process = new QRadioButton(tr("Single process mode. This mode is not officially supported by Qt WebEngine and not recommended.\nNote that single process may still make use of several threads."), this);

    settings.beginGroup ("ProcessModel");

    switch (settings.value ("type", 1).toInt())
    {
        case 0: site_per_process->setChecked(true); break;
        case 1: process_per_site->setChecked(true); break;
        case 2: single_process->setChecked(true); break;
        default: process_per_site->setChecked(true); break;
    }
    settings.endGroup();

    connect (site_per_process, &QRadioButton::toggled, [this](bool checked)
    {
        if (checked)
        {
            QSettings settings;
            settings.beginGroup ("ProcessModel");
            settings.setValue ("type", 0);
            settings.endGroup();
        }
    });


    connect (process_per_site, &QRadioButton::toggled, [this](bool checked)
    {
        if (checked)
        {
            QSettings settings;
            settings.beginGroup ("ProcessModel");
            settings.setValue ("type", 1);
            settings.endGroup();
        }
    });

    connect (single_process, &QRadioButton::toggled, [this](bool checked)
    {
        if (checked)
        {
            QSettings settings;
            settings.beginGroup ("ProcessModel");
            settings.setValue ("type", 2);
            settings.endGroup();
        }
    });

    QVBoxLayout* mp_layout = new QVBoxLayout (this);
    mp_layout->addWidget (site_per_process);
    mp_layout->addWidget (process_per_site);
    mp_layout->addWidget (single_process);
    mp->setLayout (mp_layout);


    QGroupBox* web = new QGroupBox (tr("Web settings (applied on restart)"));

    settings.beginGroup ("Web settings");
    QCheckBox* tcp_fast_open = new QCheckBox (tr("Enable TCP Fast Open, this is generally safe and results in a faster response."), this);
    if (settings.value ("tcp_fast_open", false).toBool())
        tcp_fast_open->setChecked(true);

    QCheckBox* checker_imaging = new QCheckBox (tr("Enable Checker Imaging feature, which is supposed to improve loading speeds. Your mileage may vary."), this);
    if (settings.value ("checker_imaging", false).toBool())
        checker_imaging->setChecked(true);

    settings.endGroup();


    connect (tcp_fast_open, &QCheckBox::toggled, [this](bool checked)
    {
        QSettings settings;
        settings.beginGroup ("Web settings");
        if (checked)
        {
            settings.setValue ("tcp_fast_open", true);
        }
        else
        {
            settings.setValue ("tcp_fast_open", false);
        }
        settings.endGroup();
    });

    connect (checker_imaging, &QCheckBox::toggled, [this](bool checked)
    {
        QSettings settings;
        settings.beginGroup ("Web settings");
        if (checked)
        {
            settings.setValue ("checker_imaging", true);
        }
        else
        {
            settings.setValue ("checker_imaging", false);
        }
        settings.endGroup();
    });

    QVBoxLayout* web_layout = new QVBoxLayout (this);
    web_layout->addWidget (tcp_fast_open);
    web_layout->addWidget (checker_imaging);
    web->setLayout (web_layout);


    QGroupBox* rasterizer = new QGroupBox (tr("Rasterizing (applied on restart)"), this);

    settings.beginGroup ("Rasterizer");
    QCheckBox* gpu_rasterizer = new QCheckBox (tr("Enable GPU rasterization of web pages. This is usually faster, but may also introduce delayed response.\nThere's no best answer here."), this);
    if (settings.value ("gpu_enabled", false).toBool())
        gpu_rasterizer->setChecked(true);
    /*
    QCheckBox *zero_copy = new QCheckBox (tr("Enable faster zero-copy implementation for CPU-rasterized pages.\nIdeally, this will allow the rasterizer to switch between GPU and zero-copy implementation based on the content.\nThis is currently in development, typical problems you may face: rendering crash when switching to console and back; black view if the option above was not enabled."), this);
    if (settings.value ("zero_copy", false).toBool())
        zero_copy->setChecked(true);
    */

    settings.endGroup();

    connect (gpu_rasterizer, &QCheckBox::toggled, [this](bool checked)
    {
        QSettings settings;
        settings.beginGroup ("Rasterizer");
        if (checked)
        {
            settings.setValue ("gpu_enabled", true);
        }
        else
        {
            settings.setValue ("gpu_enabled", false);
        }
        settings.endGroup();
    });
    /*
    connect (zero_copy, &QCheckBox::toggled, [this](bool checked)
    {
        QSettings settings;
        settings.beginGroup ("Rasterizer");
        if (checked)
        {
            settings.setValue ("zero_copy", true);
        }
        else
        {
            settings.setValue ("zero_copy", false);
        }
        settings.endGroup();
    });
    */

    QVBoxLayout* rasterizer_layout = new QVBoxLayout (this);
    rasterizer_layout->addWidget (gpu_rasterizer);
    //rasterizer_layout->addWidget (zero_copy);
    rasterizer->setLayout (rasterizer_layout);

    QGroupBox* experimental = new QGroupBox (tr("Expert settings (applied on restart)"), this);

    settings.beginGroup ("Experimental");
    QCheckBox* ignore_gpu_blacklist = new QCheckBox (tr("Force GPU acceleration. Note that in most cases web tabs will be accelerated anyway, but this may help with video decoding.\nMay as well have no real effect."), this);
    if (settings.value ("ignore_gpu_blacklist", false).toBool())
       ignore_gpu_blacklist->setChecked(true);
    /*
    QCheckBox *enable_gpu_buffers = new QCheckBox (tr("Enabled hardware-accelerated GPU memory buffers. Note this will have no real effect on most systems despite being reported as enabled."), this);
    if (settings.value ("enable_gpu_buffers", false).toBool())
        enable_gpu_buffers->setChecked(true);
    */
    settings.endGroup();

    connect (ignore_gpu_blacklist, &QCheckBox::toggled, [this](bool checked)
    {
        QSettings settings;
        settings.beginGroup ("Experimental");
        if (checked)
        {
            settings.setValue ("ignore_gpu_blacklist", true);
        }
        else
        {
            settings.setValue ("ignore_gpu_blacklist", false);
        }
        settings.endGroup();
    });
    /*
    connect (enable_gpu_buffers, &QCheckBox::toggled, [this](bool checked)
    {
        QSettings settings;
        settings.beginGroup ("Experimental");
        if (checked)
        {
            settings.setValue ("enable_gpu_buffers", true);
        }
        else
        {
            settings.setValue ("enable_gpu_buffers", false);
        }
        settings.endGroup();
    });
    */

    QVBoxLayout* experimental_layout = new QVBoxLayout (this);
    experimental_layout->addWidget (ignore_gpu_blacklist);
    //experimental_layout->addWidget (enable_gpu_buffers);
    experimental->setLayout (experimental_layout);

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
    layout->addWidget (visual);
    layout->addWidget (mp);
    layout->addWidget (web);
    layout->addWidget (rasterizer);
    layout->addWidget (experimental);
    layout->addWidget (content_filters);
    layout->addStretch (1);
    setLayout(layout);
}