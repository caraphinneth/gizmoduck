#pragma once
#include <QWebEngineUrlRequestInterceptor>
#include <memory>

struct intelligent_resolver_record: public QHash <QString, uint>
{

};

struct intelligent_resolver_class: public QHash <QWebEngineUrlRequestInfo::ResourceType, intelligent_resolver_record*>
{

};

struct intelligent_resolver_data: public QHash <QUrl, intelligent_resolver_class*>
{

};

struct whitelist_record
{
    whitelist_record (QWebEngineUrlRequestInfo::ResourceType t, const QString& p, const QString& s) : type(t), pattern(p), specific(s) {}

    QWebEngineUrlRequestInfo::ResourceType type;
    QString pattern;
    QString specific;
};

struct RequestFilter: public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    RequestFilter (QObject* parent = nullptr, const QString& useragent = "Googlebot/2.1 (+http://www.google.com/bot.html)");
    void interceptRequest (QWebEngineUrlRequestInfo& info);

signals:
    void debug_message (const QString& text);

public slots:
    void ReloadLists();

private:
    QString user_agent;
    std::unique_ptr<intelligent_resolver_data> intelligent_resolver;
    void intelligent_resolver_data_update (const QUrl& url, const QString& domain, const QWebEngineUrlRequestInfo::ResourceType type);
    int levenshtein_distance (const QString& str1, const QString& str2) const;
    void load_filters();

    bool should_block (QWebEngineUrlRequestInfo& info);
    QList <whitelist_record> whitelist;
};