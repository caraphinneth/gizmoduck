#include <QGridLayout>
#include <QMovie>

#include "file_dialog.h"

// Custom file dialog with image preview for uploads.
FileDialog::FileDialog (QWidget *parent, const QString &caption, const QString &directory, const QString &filter) : QFileDialog (parent, caption, directory, filter)
{
    setObjectName ("PreviewFileDialog");
    QVBoxLayout *box = new QVBoxLayout();

    preview = new QLabel (tr("Preview"), this);
    preview->setAlignment (Qt::AlignCenter);
    preview->setObjectName ("labelPreview");
    preview->setMinimumSize (256,256);
    resize (size().width()+256, size().height());
    box->addWidget (preview);
    box->addStretch();
    preview->hide();
    // add to QFileDialog layout
    {
        QGridLayout *layout = (QGridLayout*)this->layout();
        layout->addLayout (box, 1, 3, 3, 1);
    }
    connect (this, &QFileDialog::currentChanged, this, &FileDialog::OnCurrentChanged);
}

void FileDialog::OnCurrentChanged (const QString &path)
{
    QPixmap pixmap = QPixmap (path);
    if (pixmap.isNull())
    {
        /*
        QMovie *movie = new QMovie (path);
        if (movie->isValid())
        {
            //movie->setScaledSize (QSize(preview->width(), preview->height()));
            preview->setMovie (movie);
            preview->show();
            movie->start();

        }
        else
        */
        preview->hide();
        //movie->deleteLater();
    }
    else
    {
        preview->setPixmap (pixmap.scaled (preview->width(), preview->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        preview->show();
    }

}
