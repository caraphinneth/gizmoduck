#include <QAuthenticator>
#include <QIcon>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTimer>
#include "webpage.h"
#include "file_dialog.h"

WebPage::WebPage (QWebEngineProfile *profile, QWidget *parent): QWebEnginePage (profile, parent)
{
    connect(this, &QWebEnginePage::authenticationRequired, this, &WebPage::handleAuthenticationRequired);

    lifecycle = new QTimer (this);
    connect(this, &QWebEnginePage::recommendedStateChanged, this, [this]() {
        if (recommendedState()==QWebEnginePage::LifecycleState::Active)
            lifecycle->start (1);
        else if  (!isVisible())
        {
            if (recommendedState()==QWebEnginePage::LifecycleState::Frozen)
                lifecycle->start (15*60*1000);
            else if (recommendedState()==QWebEnginePage::LifecycleState::Discarded)
                lifecycle->start (15*60*1000);
        }
    });

    connect (this, &QWebEnginePage::lifecycleStateChanged, [this](QWebEnginePage::LifecycleState state)
    {
        if (state == QWebEnginePage::LifecycleState::Discarded)
            emit iconChanged (QIcon (QStringLiteral (":/icons/sleep")));
        else if (state == QWebEnginePage::LifecycleState::Frozen)
            emit iconChanged (QIcon (QStringLiteral (":/icons/freeze")));
        else
            emit iconChanged (icon());
    });

    connect(lifecycle, &QTimer::timeout, this, [this]() {
        if (lifecycleState() != recommendedState())
        {
            setLifecycleState (recommendedState());
        }
    });
}

//TODO: args
QStringList WebPage::chooseFiles (QWebEnginePage::FileSelectionMode mode, const QStringList&/*oldFiles*/, const QStringList&/*acceptedMimeTypes*/)
{
    QStringList list;
    FileDialog *dialog = new FileDialog (view(), tr("Select File"), "", tr("All Files (*.*)"));
    dialog->setAcceptMode (QFileDialog::AcceptOpen);

    if (mode == QWebEnginePage::FileSelectOpen)
        dialog->setFileMode (QFileDialog::ExistingFile);
    else if (mode == QWebEnginePage::FileSelectOpenMultiple)
        dialog->setFileMode (QFileDialog::ExistingFiles);

    if (dialog->exec())
        list = dialog->selectedFiles();
    else list << "";
    dialog->deleteLater();
    return list;
}

void WebPage::handleAuthenticationRequired (const QUrl &url, QAuthenticator *auth)
{
    QWidget *mainWindow = view()->window();

    QDialog dialog (mainWindow);
    dialog.setModal (true);
    dialog.setWindowFlags (dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QLabel *auth_label = new QLabel (tr ("Enter username and password for \"%1\" at %2").arg (auth->realm()).arg(url.toString().toHtmlEscaped()));
    auth_label->setWordWrap (true);

    QLineEdit *username = new QLineEdit (&dialog);
    QLineEdit *password = new QLineEdit (&dialog);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget (auth_label);
    layout->addWidget (username);
    layout->addWidget (password);
    dialog.setLayout(layout);

    connect (password, &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted)
    {
        auth->setUser (username->text());
        auth->setPassword (password->text());
    }
    else
    {
        *auth = QAuthenticator();
    }
    auth_label->deleteLater();
    username->deleteLater ();
    password->deleteLater ();

}


