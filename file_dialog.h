// File upload dialog, extended with image preview.

#pragma once
#include <QFileDialog>
#include <QLabel>

struct FileDialog: public QFileDialog
{
    Q_OBJECT

    public:

    FileDialog (QWidget* parent=nullptr, const QString& caption = QString(), const QString& directory = QString(), const QString& filter = QString());

    protected slots:

    void OnCurrentChanged (const QString& path);

    protected:

    QLabel* preview;
};
