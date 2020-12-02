#include <QAuthenticator>
#include <QIcon>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>

#include "webpage.h"
#include "webview.h"
#include "file_dialog.h"

WebPage::WebPage (QWebEngineProfile* profile, QWidget* parent): QWebEnginePage (profile, parent)
{
    old_url = url();
    connect(this, &QWebEnginePage::authenticationRequired, this, &WebPage::handleAuthenticationRequired);

    lifecycle = new QTimer (this);
    connect(this, &QWebEnginePage::recommendedStateChanged, this, [this]() {
        if (recommendedState()==QWebEnginePage::LifecycleState::Active)
            //lifecycle->start (1);
            setLifecycleState (QWebEnginePage::LifecycleState::Active);
        else if (!isVisible())
        {
            if (recommendedState()==QWebEnginePage::LifecycleState::Frozen)
                lifecycle->start (15*60*1000);
            else if ((recommendedState()==QWebEnginePage::LifecycleState::Discarded)&&
                     (url().host()!="discord.com")&&
                     (url().host()!="tripwire.eve-apps.com")&&
                     (url().host()!="colab.research.google.com")
                     )
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
            qDebug() << "State for" << url() << "changed to" << lifecycleState ();
        }
    });
}

//TODO: args
QStringList WebPage::chooseFiles (QWebEnginePage::FileSelectionMode mode, const QStringList&/*oldFiles*/, const QStringList&/*acceptedMimeTypes*/)
{
    QStringList list;
    QScopedPointer <FileDialog> dialog;
    dialog. reset (new FileDialog (view(), tr("Select File"), "", tr("All Files (*.*)")));
    dialog->setAcceptMode (QFileDialog::AcceptOpen);

    if (mode == QWebEnginePage::FileSelectOpen)
        dialog->setFileMode (QFileDialog::ExistingFile);
    else if (mode == QWebEnginePage::FileSelectOpenMultiple)
        dialog->setFileMode (QFileDialog::ExistingFiles);

    if (dialog->exec())
        list = dialog->selectedFiles();
    else list << "";
    //dialog->deleteLater();
    return list;
}

void WebPage::handleAuthenticationRequired (const QUrl& url, QAuthenticator* auth)
{
    QWidget *mainWindow = view()->window();

    QDialog dialog (mainWindow);
    dialog.setModal (true);
    dialog.setWindowFlags (dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QScopedPointer <QLabel> auth_label;
    auth_label.reset (new QLabel (tr ("Enter username and password for \"%1\" at %2").arg (auth->realm()).arg(url.toString().toHtmlEscaped())));
    auth_label->setWordWrap (true);

    QScopedPointer<QLineEdit> username;
    username.reset (new QLineEdit (&dialog));
    QScopedPointer<QLineEdit> password;
    password.reset (new QLineEdit (&dialog));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget (auth_label.data());
    layout->addWidget (username.data());
    layout->addWidget (password.data());
    dialog.setLayout(layout);

    connect (password.data(), &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted)
    {
        auth->setUser (username->text());
        auth->setPassword (password->text());
    }
    else
    {
        *auth = QAuthenticator();
    }
}

bool WebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
    if (isMainFrame)
        qDebug() << "Going to url" << url << "by navitype" << type;
    //else
       // qDebug() << "NON_MAINFRAME request of url" << url << "by navitype" << type;
    //||(type == QWebEnginePage::NavigationTypeTyped)
    if (((type == QWebEnginePage::NavigationTypeLinkClicked)) && isMainFrame)
    {
        WebView* v = qobject_cast<WebView*>(view());
        if (v)
        {
            // emit v->loadFinished (false);
            emit v->link_requested (url.toString(), false);
            return false;
        }
    }

    /*else if (((type == QWebEnginePage::NavigationTypeRedirect) && isMainFrame))// && (this->url().host() != url.host()))
    {
        WebView* v = qobject_cast<WebView*>(view());
        if (v)
        {
            qDebug() << "Intercepting" << url << "request by" << this->url().host();
            emit v->link_requested (url.toString());
            return false;
        }
    }*/

    /*
    else if ((type == QWebEnginePage::NavigationTypeFormSubmitted) && isMainFrame)// && (this->url().host() != url.host()) && isMainFrame)
    {
        WebView* v = qobject_cast<WebView*>(view());
        if (v)
        {
            qDebug() << "Intercepting" << url << "request by" << this->url().host();
            emit v->link_requested (url.toString());
            return false;
        }
    }*/
    /*
    else if ((type == QWebEnginePage::NavigationTypeTyped) && (this->url().host() != url.host()) && isMainFrame)
    {
        WebView* v = qobject_cast<WebView*>(view());
        if (v)
        {
            emit v->link_requested (url.toString());
            return false;
        }
    }*/
    return true;
}

