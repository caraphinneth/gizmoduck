#pragma once
#include <QWebEnginePage>

#include "file_dialog.h"

struct WebPage : public QWebEnginePage
{
    Q_OBJECT

public:
    WebPage (QWebEngineProfile* profile, QWidget* parent = nullptr);
    ~WebPage ();
    QTimer* lifecycle;
    QUrl old_url;

private slots:
    void handleAuthenticationRequired (const QUrl& url, QAuthenticator* auth);

protected:
    QStringList chooseFiles (QWebEnginePage::FileSelectionMode mode, const QStringList& oldFiles, const QStringList& acceptedMimeTypes) override; // Overload to implement non-useless file dialog.
    bool acceptNavigationRequest (const QUrl& url, NavigationType type, bool isMainFrame) override;
};
