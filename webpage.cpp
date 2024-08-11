#include <QAuthenticator>
#include <QIcon>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEngineHistory>

#include "file_dialog.h"
#include "webpage.h"
#include "webview.h"

WebPage::WebPage (QWebEngineProfile* profile, QWidget* parent): QWebEnginePage (profile, parent)
{
    old_url = url();

    connect(this, &WebPage::authenticationRequired, this, &WebPage::handleAuthenticationRequired);

    lifecycle = new QTimer (this);
    connect(this, &WebPage::recommendedStateChanged, [this]()
    {
        qDebug() << "State change advised:" << recommendedState();
        if (recommendedState()==QWebEnginePage::LifecycleState::Active)
        {
            //lifecycle->start (1);
            setLifecycleState (QWebEnginePage::LifecycleState::Active);
        }
        else if (!isVisible()&&
                 (url().host()!="discord.com")&&
                 (url().host()!="twitter.com")&&
                 (url().host()!="tripwire.eve-apps.com")&&
                 (url().host()!="colab.research.google.com")&&
                 (url().host()!="localhost")
                 )
        {
            if (recommendedState()==QWebEnginePage::LifecycleState::Frozen)
                lifecycle->start (15*60*1000);
            else if (recommendedState()==QWebEnginePage::LifecycleState::Discarded)
                lifecycle->start (15*60*1000);
        }
    });

    connect (this, &WebPage::lifecycleStateChanged, [this, profile](QWebEnginePage::LifecycleState state)
    {
        if (state == QWebEnginePage::LifecycleState::Discarded)
            emit iconChanged(QIcon(QStringLiteral (":/icons/sleep")));
        else if (state == QWebEnginePage::LifecycleState::Frozen)
            emit iconChanged(QIcon(QStringLiteral (":/icons/freeze")));

        else
            profile->requestIconForPageURL(url(), 16, [this](const QIcon &icon, const QUrl, const QUrl)
            {
                if (icon.isNull())
                {
                    emit iconChanged(QIcon(QStringLiteral(":/icons/gizmoduck")));
                }
                else
                {
                    emit iconChanged(icon);
                }
            });

     });

    connect (lifecycle, &QTimer::timeout, [this]()
    {
        if (lifecycleState() != recommendedState())
        {
            setLifecycleState (recommendedState());
            qDebug() << "State for" << url() << "changed to" << lifecycleState();
        }
    });
}

WebPage::~WebPage()
{
    lifecycle->stop();
    delete(lifecycle);
    //QWebEnginePage::~QWebEnginePage();
}

//TODO: args
QStringList WebPage::chooseFiles (QWebEnginePage::FileSelectionMode mode, const QStringList&/*oldFiles*/, const QStringList&/*acceptedMimeTypes*/)
{
    QStringList list;
    QScopedPointer <FileDialog> dialog;
    dialog.reset (new FileDialog (QWebEngineView::forPage(this), tr("Select File"), "", tr("All Files (*.*)")));
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
    QWidget* mainWindow = QWebEngineView::forPage(this)->window();

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

    QVBoxLayout* layout = new QVBoxLayout;
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
    else
        qDebug() << "NON_MAINFRAME request of url" << url << "by navitype" << type;
    //||(type == QWebEnginePage::NavigationTypeTyped)
    if (((type == QWebEnginePage::NavigationTypeLinkClicked)) && isMainFrame)
    {
        if (WebView* view = qobject_cast<WebView*>(QWebEngineView::forPage(this)))
        {
            //qDebug() << "Clicked the link" << url.toString();
            emit view->link_requested (url.toString(), false);
            return false;
        }
    }

   /* else if (((type == QWebEnginePage::NavigationTypeRedirect) && isMainFrame))// && (this->url().host() != url.host()))
    {
        WebView* v = qobject_cast<WebView*>(QWebEngineView::forPage(this));
        if (v)
        {
            qDebug() << "Intercepting" << url << "request by" << this->url().host();
            emit v->link_requested (url.toString(), false);
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

