#include <QChar>
#include <QDir>
#include <QHostAddress>
#include <QRegularExpression>
#include <QSettings>

#include "request_filter.h"

QList <whitelist_record> load_filter_from_file (const QString& filename, QWebEngineUrlRequestInfo::ResourceType type)
{
    QList <whitelist_record> ret;
    QFile file (filename);
    if (file.open (QIODevice::ReadOnly))
    {
       QTextStream in(&file);
       while (!in.atEnd())
       {
           QString s = in.readLine();
           if (!s.startsWith ("#"))
               ret.append (whitelist_record (type, s, QString()));
       }
       file.close();
    }
    return ret;
}

QStringList TLD (const QString& value)
{
    QStringList list = value.split ('.');
    while (list.count() > 2) list.removeFirst();
    return list;
}

QStringList SLD (const QString& value)
{
    QStringList list = value.split ('.');
    while (list.count() > 3) list.removeFirst();
    return list;
}

bool SameUpperDomain (const QString& v1, const QString& v2)
{
    return (TLD(v1) == TLD(v2));
}

bool SameSecondLevelDomain (const QString& v1, const QString& v2)
{
    return (SLD(v1) == SLD(v2));
}

bool match (const QUrl& url, const QString& pattern)
{
    int i = pattern.indexOf ('/'); // first slash, if any, otherwise assume it's a domain name
    if  (i == -1) // assuming it's a domain name
    {
        int dots = pattern.count(".");
        if (dots == 1) // assuming TLD+1
        {
            return SameUpperDomain (url.host(), pattern);
        }
        else if (dots == 2) // check TLD+2 matching, which is safer than just contains()
        {
            return SameSecondLevelDomain (url.host(), pattern);
        }
        else
        {
            // Stub, we should not rely on such patterns...
            return false;
        }
    }
    else
    {
        // Split pattern into domain and path
        QString domain = pattern.left (i);
        QString path = pattern.right (pattern.length() - i);
        // For now we use filters
        //qDebug() << "Domain: " << domain << " host: " << url.host();
        //qDebug() << "Path: " << path << " left side: " << url.path();

        return (SameUpperDomain(domain, url.host())&&(url.path().startsWith(path)));
    }
}

QString ResourceClass (const QWebEngineUrlRequestInfo::ResourceType type)
{
    QString resource_type;
    switch (type)
    {
    case QWebEngineUrlRequestInfo::ResourceTypeMainFrame: resource_type = "main frame"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeSubFrame	: resource_type = "subframe"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeStylesheet: resource_type = "CSS"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeScript: resource_type = "script"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeImage: resource_type = "image"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeFontResource: resource_type = "font"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeSubResource: resource_type = "subresource"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeObject: resource_type = "object"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeMedia: resource_type = "media"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeWorker: resource_type = "dedicated worker"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeSharedWorker: resource_type = "shared worker"; break;
    case QWebEngineUrlRequestInfo::ResourceTypePrefetch: resource_type = "prefetch"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeFavicon: resource_type = "favicon"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeXhr: resource_type = "XMLHttpRequest"; break;
    case QWebEngineUrlRequestInfo::ResourceTypePing: resource_type = "ping"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeServiceWorker: resource_type = "service worker"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeCspReport: resource_type = "policy report (you should not GET this)"; break;
    case QWebEngineUrlRequestInfo::ResourceTypePluginResource: resource_type = "plugin reaching out"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeNavigationPreloadMainFrame: resource_type = "main frame preload service worker"; break;
    case QWebEngineUrlRequestInfo::ResourceTypeNavigationPreloadSubFrame: resource_type = "subframe preload service worker"; break;

    default: resource_type = "UNKNOWN (websocket connection?)";
    }
    return resource_type;
}

QWebEngineUrlRequestInfo::ResourceType ResourceClass (const QString& type)
{
    if (type == "main frame") return QWebEngineUrlRequestInfo::ResourceTypeMainFrame;
    if (type == "subframe") return QWebEngineUrlRequestInfo::ResourceTypeSubFrame;
    if (type == "CSS") return QWebEngineUrlRequestInfo::ResourceTypeStylesheet;
    if (type == "script") return QWebEngineUrlRequestInfo::ResourceTypeScript;
    if (type == "image") return QWebEngineUrlRequestInfo::ResourceTypeImage;
    if (type == "font") return QWebEngineUrlRequestInfo::ResourceTypeFontResource;
    if (type == "subresource") return QWebEngineUrlRequestInfo::ResourceTypeSubResource;
    if (type == "object") return QWebEngineUrlRequestInfo::ResourceTypeObject;
    if (type == "media") return QWebEngineUrlRequestInfo::ResourceTypeMedia;
    if (type == "dedicated worker") return QWebEngineUrlRequestInfo::ResourceTypeWorker;
    if (type == "shared worker") return QWebEngineUrlRequestInfo::ResourceTypeSharedWorker;
    if (type == "prefetch") return QWebEngineUrlRequestInfo::ResourceTypePrefetch;
    if (type == "favicon") return QWebEngineUrlRequestInfo::ResourceTypeFavicon;
    if (type == "XMLHttpRequest") return QWebEngineUrlRequestInfo::ResourceTypeXhr;
    if (type == "ping") return QWebEngineUrlRequestInfo::ResourceTypePing;
    if (type == "service worker") return QWebEngineUrlRequestInfo::ResourceTypeServiceWorker;
    if (type == "policy report") return QWebEngineUrlRequestInfo::ResourceTypeCspReport;
    if (type == "plugin reaching out") return QWebEngineUrlRequestInfo::ResourceTypePluginResource;
    if (type == "main frame preload service worker") return QWebEngineUrlRequestInfo::ResourceTypeNavigationPreloadMainFrame;
    if (type == "subframe preload service worker") return QWebEngineUrlRequestInfo::ResourceTypeNavigationPreloadSubFrame;
    return QWebEngineUrlRequestInfo::ResourceTypeMainFrame;
}

RequestFilter::RequestFilter (QObject* parent, const QString& useragent): QWebEngineUrlRequestInterceptor (parent)
{
    user_agent = useragent;
    intelligent_resolver.reset (new intelligent_resolver_data());
    ReloadLists();
}

void RequestFilter::load_filters()
{
    QSettings settings;
    settings.beginGroup ("Filtering");
    QStringList enabled_filters = settings.value ("enabled").toStringList();
    qDebug() << enabled_filters;
    settings.endGroup();

    foreach (const QString& filename, enabled_filters)
    {
        QWebEngineUrlRequestInfo::ResourceType guess_type = QWebEngineUrlRequestInfo::ResourceTypeMainFrame;
        int i = filename.indexOf ("-");
        if (i != -1)
            guess_type = ResourceClass (filename.left(i));

        whitelist.append (load_filter_from_file ("./whitelist/" + filename, guess_type));
        qDebug() << "Filename: " << filename << " guessed class: " << ResourceClass(guess_type);
    }
}

void RequestFilter::ReloadLists()
{
    if (!whitelist.empty())
        whitelist.clear();
    load_filters ();
}

void RequestFilter::interceptRequest (QWebEngineUrlRequestInfo& info)
{
    // Intercepting the requested URL
    QString url = info.requestUrl().toString (QUrl::RemoveQuery);

    // Never track me you fucking cunt
    info.setHttpHeader("DNT", "1");

    // Per-domain quirks
    if (info.requestUrl().host()=="accounts.google.com")
    {
        QString edge_agent = user_agent + " Mozilla/5.0 ({os_info}; rv:90.0) Gecko/20100101 Firefox/90.0";
        info.setHttpHeader ("User-Agent", edge_agent.toUtf8());
    }

    if (!should_block (info))
    {
        info.block (false);
    }
    else
    {
        //info.block (true);
        info.redirect (QUrl::fromUserInput ("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII="));
    }
}

bool RequestFilter::should_block (QWebEngineUrlRequestInfo& info)
{
    QString source_host = info.firstPartyUrl().host();
    QString destination_host = info.requestUrl().host();
    QString url = info.requestUrl().toString (QUrl::RemoveQuery);

    // Prevent recursion.
    if (url == "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=")
    {
        return false;
    }
    // Always block certain request types.
    /*
    else if
    (
            //(info.resourceType()==(QWebEngineUrlRequestInfo::ResourceTypePrefetch))||
            (info.resourceType()==(QWebEngineUrlRequestInfo::ResourceTypePing))
    )
    {
        return true;
        debug_message ("Requested " + ResourceClass (info.resourceType ())+" "+url+" blocked.");
    }
    */

    // Always allow certain request types.
    if
    (
            (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame)||
            (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeFavicon)||
            (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeFontResource)||
            (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeStylesheet)
    )
    {
        // debug_message ("Requested " + ResourceClass (info.resourceType ())+" "+url+" allowed.");
        return false;
    }

    // If the destination has a special scheme.
    if (url.startsWith ("chrome-extension:"))
    {
            return false;
            debug_message ("This is an extension request, allowing for now.");
    }


    if (destination_host.isEmpty())
    {
        QString message = "Unable to decode host from URL ("+url+"), looking deeper... ";
        if
        (
                url.startsWith ("data:")||
                url.startsWith ("blob:")||
                url.startsWith ("file:")
        )
        {
                return false;
                debug_message (message + "This is an inline data/binary/file request, allowing for now.");
        }
    }

    // Requested resource has the same origin.
    if (SameUpperDomain (destination_host, source_host))
    {
        // Gather stats yet!
        intelligent_resolver_data_update (info.firstPartyUrl(), TLD (destination_host).join("."), info.resourceType());

        if
        // First-party ads.
        (
            url.contains ("/banners/")||
            url.contains ("/banner/")||
            url.contains ("pagead")||
            url.contains ("/ad_companion")||
            url.contains ("/adv/")||
            url.contains ("/ads/")||
            url.contains ("/ptracking")||
            (url.contains ("/get_video_info")&&(source_host=="www.youtube.com"))|| // This one's complicated... youtube uses this to deliver ads when first-party, but not remotely. Can break youtube gaming?
            //url.contains ("/cthru")||
            destination_host.startsWith ("ad.")||
            destination_host.startsWith ("ads.")||
            destination_host.startsWith ("ads-")

        )
        {
            debug_message (url + ": same origin, but suspicious path/subdomain. Attempting to block.");
            return true;
        }

        // All clear: first-party and not blacklisted.
        else
        {
            //debug_message (source_host + " and " + destination_host + " - same point of origin, content allowed. If it is not so, report a bug. URL: "+url);
            return false;
        }

    }

    // Global whitelist before the stats!

    foreach (const whitelist_record& record, whitelist)
    {
        if ((record.type == QWebEngineUrlRequestInfo::ResourceTypeMainFrame)||(record.type == info.resourceType()))
                if (match (info.requestUrl(), record.pattern))
                {
                    // debug_message ("New filter: " + ResourceClass (info.resourceType ())+" from " + destination_host + " requested by " + source_host + " whitelisted, precise URL: "+url);
                    return false;
                }
    }

    if
    (
    (info.requestUrl().scheme()=="chrome")|| // Whitelist internals.

    (source_host=="toloka.yandex.ru")||(destination_host=="toloka.yandex.ru")||

    ((destination_host.contains ("accounts.google.com")|| (destination_host.contains ("clients")&& SameUpperDomain (destination_host, "google.com")))&&source_host.isEmpty())||
    ((destination_host=="www.google.com")&&source_host.isEmpty())||

    ((SameUpperDomain (destination_host, "ttvnw.net")&&SameUpperDomain (source_host, "twitch.tv")))||

    destination_host.contains("maps.yandex.")||

    // Allow embedding vimeo

    ((url.contains ("daxab.com/player"))&&(!url.contains ("fuckadblock")))|| // Daxab embedded player

    //(info.resourceType()==(QWebEngineUrlRequestInfo::ResourceTypeSubFrame)&&SameUpperDomain (destination_host, "vk.com"))||

    //SameUpperDomain (destination_host, "entervideo.net")||destination_host.contains("185.176.192.")||

    // SameUpperDomain(destination_host,"fontawesome.com")||

    (SameUpperDomain(destination_host,"coub.com")&&!url.contains("banner")) || // Allow embedding coub, except the ad.

    (SameUpperDomain (destination_host, "wp.com") && ((destination_host!="pixel.wp.com")&&destination_host!="stats.wp.com"))|| // Allow wordpress hosting, but not services.
    // Mainly i?.wp.com for images, s?.wp.com is likely for scrips and may need filtering.

    destination_host.contains("bam.nr-data.net")|| // Danbooru mail.

    destination_host.contains ("qrcode.kaywa.com")|| // QR codes
    destination_host.contains ("openbenchmarking.org")||

    // SameUpperDomain (destination_host, "konggames.com")|| // this is a bad idea

    SameUpperDomain (destination_host, "paymo.ru")||
    destination_host.contains ("3dsecure.qiwi.com")||

    ((destination_host.contains ("static"))&&
        (!url.contains ("static.doubleclick.net"))&&(!url.contains ("static.addtoany.com")) )|| // Any gstatic, yastatic etc... for now.
    (url.contains ("google.com/static"))||
    SameUpperDomain (destination_host, "yastat.net")||
    (destination_host.contains ("assets"))||
    //((info.resourceType()!=QWebEngineUrlRequestInfo::ResourceTypeScript)&&
    ((destination_host.contains ("cdn")||url.contains ("/cdn/"))&&
        (!destination_host.contains ("popcash.net"))
       )|| // Placeholder for various CDNs
    (destination_host.contains("usercontent."))|| // Ideally should be preceded with a part of source host
    destination_host.startsWith("api.")

    )

    {
        // debug_message ("A " + ResourceClass (info.resourceType ())+" from " + destination_host + " requested by " + source_host + " whitelisted, precise URL: "+url);
        return false;
    }


    // Image hosts
    if (info.resourceType()==(QWebEngineUrlRequestInfo::ResourceTypeImage))

    {
        if
        (
            QAbstractSocket::IPv4Protocol == QHostAddress (destination_host).protocol()|| // Valid IP address. TODO: reverse lookup.


            // URL pattern that are likely to be embedded and unlikely to be ads:
            url.contains ("/uploads/")||
            url.contains ("/upload/")||
            url.contains ("/article")||
            url.contains ("/content/")||
            url.contains ("/sites/")||

            QRegularExpression("/20\\d\\d").match(url).hasMatch()||

            source_host.contains("google.") // Allow google image search to fetch fullsize images.
        )
        {
                // debug_message ("Favorable pattern: image " + destination_host + " requested by " + source_host + " whitelisted, precise URL: "+url);
                return false;
        }
    }

    // If the source is an inline.
    if (source_host.isEmpty())
    //(info.firstPartyUrl().toString()=="data:,")
    {
        if
        (
                // Altered for my work purposes.
                ((info.resourceType()!=QWebEngineUrlRequestInfo::ResourceTypeScript)&&
                (info.resourceType()!=QWebEngineUrlRequestInfo::ResourceTypeXhr))|| destination_host.contains ("yandex")
        )
        {
            // TODO: since first-party is unavailable here, some other restrictions should be used.
            debug_message ("Allowing " + ResourceClass (info.resourceType ()) +" request from the inline: "+url);
            return false;
        }
        else
        {
            QString message =  ResourceClass (info.resourceType()) + " requested from the inline, no thanks: "+url;
            debug_message (message + "<br/>Source URL: "+info.firstPartyUrl().toString());
            return true;
        }
    }

    // Okay, if it's third-party... we block by default and whitelist what's truly needed.
    // Always whitelist:

    if
    (
        (levenshtein_distance (TLD (source_host).join("."), TLD (destination_host).join("."))<=3)&&
        ((TLD (source_host).first().length()>3)||(TLD (destination_host).first().length()>3))
    )
    {
        debug_message ("Short L-distance, assuming same origin.");
        return false;
    }

    if
    (
        (destination_host.contains (TLD (source_host).first())||
         source_host.contains (TLD (destination_host).first()))&&
        (info.firstPartyUrl ().scheme ()!="data")
    )
    {
        debug_message ("Destination contains hostname, assuming same origin.<br/>URL: "+url+", source: "+info.firstPartyUrl ().toString ()+", TLD+1: "+TLD (source_host).first());
        return false;
    }

    // Otherwise gather stats.
    intelligent_resolver_class* group;
    intelligent_resolver_record* record;
    intelligent_resolver_data_update (info.firstPartyUrl(), TLD (destination_host).join("."), info.resourceType());
    group = intelligent_resolver->value (info.firstPartyUrl());
    record = group->value (info.resourceType());

    /* if
    (
        (float (record->value (UpperDomain (destination_host).join("."))) / record->value ("All")>= 0.67 )
    )
    {
        debug_message ("Resource grabs over 2/3 of "+ResourceClass (info.resourceType ())+" requests from "+UpperDomain (destination_host).join(".")+" - allowing!");
        return false;
    }*/

    if
    (
            (float (record->value (TLD (destination_host).join("."))) / record->value ("All")>= 0.4)&&
            //(info.resourceType ()==QWebEngineUrlRequestInfo::ResourceTypeImage)
            (info.resourceType ()!=QWebEngineUrlRequestInfo::ResourceTypeScript)&&
            (info.resourceType ()!=QWebEngineUrlRequestInfo::ResourceTypeSubFrame)&&
            (info.resourceType ()!=QWebEngineUrlRequestInfo::ResourceTypeUnknown)
    )
    {
        debug_message ("Resource grabs over 40% of "+ResourceClass (info.resourceType ())+" requests from "+TLD (destination_host).join(".")+" - allowing!");
        return false;
    }

    // Whitelist for certain sites:
    /* if
    (
        (((destination_host.contains("rambler.")||(destination_host.contains("rl0.ru"))||destination_host.contains("eagleplatform.com"))&&source_host.contains("rambler.ru")))||
        ((SameUpperDomain (destination_host, "livejournal.net")||SameUpperDomain (destination_host, "lj-toys.com")) &&SameUpperDomain (source_host, "livejournal.com"))||
        ((SameUpperDomain (destination_host, "blogblog.com")&&SameUpperDomain (source_host, "blogspot.ru")))||
        ((SameUpperDomain (destination_host, "blogger.com")&&SameUpperDomain (source_host, "blogspot.ru")))||
        (((SameUpperDomain (destination_host, "imgsmail.ru")||SameUpperDomain (destination_host, "mradx.net"))&&SameUpperDomain (source_host, "mail.ru")))||
        ((SameUpperDomain (destination_host, "rtr-vesti.ru")&&SameUpperDomain (source_host, "vesti.ru")))||
        ((SameUpperDomain (destination_host, "ttvnw.net")&&SameUpperDomain (source_host, "twitch.tv")))||
        ((SameUpperDomain (destination_host, "redd.it")&&SameUpperDomain (source_host, "reddit.com")))||
        ((SameUpperDomain (destination_host, "ehgt.org")&&SameUpperDomain (source_host, "e-hentai.org")))||
        ((SameUpperDomain (destination_host, "s-msft.com")&&SameUpperDomain (source_host, "microsoft.com")))||
        ((SameUpperDomain (source_host, "exhentai.org")||SameUpperDomain (source_host, "e-hentai.org"))&&(info.resourceType()==QWebEngineUrlRequestInfo::ResourceTypeImage))|| // Allow exhentai p2p network. TODO: only numeric urls.
        (info.firstPartyUrl().toString().startsWith("data:")&&SameUpperDomain (destination_host,"wdfiles.com"))|| // Allow empty source for wikidot for now. Need a better solution.
        (info.firstPartyUrl().toString().startsWith("data:")&&SameUpperDomain (destination_host,"scp-wiki.net"))|| // Only needed for rare format quirks.
        ((SameUpperDomain (destination_host, "fsdn.com")&&SameUpperDomain (source_host,"sourceforge.net")))||
        ((SameUpperDomain (destination_host, "pling.com")&&SameUpperDomain (source_host,"kde.org")))||
        ((SameUpperDomain (destination_host, "opendesktop.org")&&SameUpperDomain (source_host,"kde.org")))||
        ((SameUpperDomain (destination_host, "tdp.ru")&&SameUpperDomain (source_host,"petrovich.ru")))||
        ((SameUpperDomain (destination_host, "s-microsoft.com")&&SameUpperDomain (source_host,"microsoft.com")))||
        ((SameUpperDomain (destination_host, "yimg.com")&&SameUpperDomain (source_host,"flickr.com")))||
        ((SameUpperDomain (destination_host, "exmoney.com")&&SameUpperDomain (source_host,"exmo.me")))||
        ((SameUpperDomain (destination_host, "nx.com")&&SameUpperDomain (source_host,"nexon.com")))||
        ((destination_host.contains ("mayhemydg.github.io")&&SameUpperDomain (source_host,"4chan.org")))||
        (SameUpperDomain (source_host,"bato.to")&&SameUpperDomain (destination_host, "anyacg.co"))||
        (SameUpperDomain (source_host,"vk.com")&&SameUpperDomain (destination_host, "pp.userapi.com"))||
        (SameUpperDomain (source_host,"yahoo.com")&&SameUpperDomain (destination_host, "yimg.com"))||
        (SameUpperDomain (source_host,"undocs.org")&&SameUpperDomain (destination_host, "un.org"))||
        (SameUpperDomain (destination_host, "adultmanga.me")&&SameUpperDomain (source_host,"mintmanga.com"))||
        (SameUpperDomain (destination_host, "neweggimages.com")&&SameUpperDomain (source_host,"newegg.com"))||
        (SameUpperDomain (destination_host, "discord.gg")&&SameUpperDomain (source_host,"discordapp.com"))||
        (SameUpperDomain (destination_host, "wvservices.com")&&SameUpperDomain (source_host,"wavesplatform.com"))||
        (SameUpperDomain (destination_host, "ap.lijit.com")&&SameUpperDomain (source_host,"speedtypingonline.com"))||
        (SameUpperDomain (destination_host, "qnssl.com")&&SameUpperDomain (source_host,"candy.one"))||
        //(SameUpperDomain (destination_host, "nicoseiga.jp")&&SameUpperDomain (source_host,"nicovideo.jp"))||
        (SameUpperDomain (destination_host, "vwp.su")&&SameUpperDomain (source_host,"voffka.com"))||
        (SameUpperDomain (destination_host, "blogimg.jp")&&SameUpperDomain (source_host,"ldblog.jp"))||
        (SameUpperDomain (destination_host, "rambler.ru")&&SameUpperDomain (source_host,"championat.com"))||
        (SameUpperDomain (destination_host, "pusher.com")&&SameUpperDomain (source_host,"wex.nz"))||
        (SameUpperDomain (destination_host, "ljplus.ru")&&SameUpperDomain (source_host,"livejournal.com"))||
        (SameUpperDomain (destination_host, "ngfiles.com")&&SameUpperDomain (source_host,"newgrounds.com"))||
        (SameUpperDomain (destination_host, "afgr2.com")&&SameUpperDomain (source_host,"newgrounds.com"))||
        (SameUpperDomain (destination_host, "wixmp.com")&&SameUpperDomain (source_host,"deviantart.com"))||
        (SameUpperDomain (destination_host, "bafang.com")&&SameUpperDomain (source_host,"okex.com"))||
        (SameUpperDomain (destination_host, "aliyun.com")&&SameUpperDomain (source_host,"uex.com"))||
        (SameUpperDomain (destination_host, "poloniex.netverify.com")&&SameUpperDomain (source_host,"poloniex.com"))
    )
    {
        debug_message ("Third-party content from " + destination_host + " whitelisted exclusively for " + source_host);
        debug_message ("Resource type: "+ResourceClass (info.resourceType ()));
        return false;
    }
    */
    // Some 'subresource' data has no first party url associated (script-generated and stuff). I assume it is safe to allow.

    /*else
    {
        if (source_host.isEmpty())

        (
                    (info.resourceType()==QWebEngineUrlRequestInfo::ResourceTypeSubResource)||
                    SameUpperDomain (destination_host, "crazycloud.ru")||
                    destination_host.contains ("yandex")||
                    url.contains("blob://")
        ))
        {
            debug_message ("Subresource " +url + " requested from inline, unable to track! Skipping.");
            return false;
        }

        // Otherwise block with extreme prejudice!
        else
        {
            debug_message ("Non-whitelisted third-party content from " + destination_host+ " requested by " + source_host + " - blocked. Precise URL: " + url);
            if (source_host.isEmpty())
                debug_message ("Request origin "+info.firstPartyUrl().toString()+" untrackable, likely an inline script. Source not whitelisted, forbidden. URL: "+url);
            debug_message ("Resource type: "+ResourceClass (info.resourceType ()));
            return true;
        }
    }*/

    debug_message ("Non-whitelisted third-party content from " + destination_host+ " requested by " + source_host + " - blocked. Precise URL: " + url);
    return true;
}


void RequestFilter::intelligent_resolver_data_update (const QUrl& url, const QString& domain, const QWebEngineUrlRequestInfo::ResourceType type)
{
    if (intelligent_resolver->contains (url))
    {
        intelligent_resolver_class *group = intelligent_resolver->value (url);

        if (group->contains (type))
        {
            intelligent_resolver_record *record = group->value (type);

            if (record->contains (domain))
            {
                record->insert (domain, record->value (domain)+1);
            }
            else
            {
                record->insert (domain, 1);
            }
            uint summary = record->value ("All");
            record->insert ("All", summary+1);

            if (!SameUpperDomain (url.host(), domain))
                debug_message (url.toString()+" addressed "+domain+" "+QString::number(record->value (domain))+" times. Total "+ResourceClass (type)+" requests from this URL: "+QString::number(record->value ("All")));
        }
        else
        {
            intelligent_resolver_record *record = new intelligent_resolver_record ();
            record->insert (domain, 1);
            record->insert ("All",1);
            group->insert (type, record);
            intelligent_resolver->insert (url, group);
            if (!SameUpperDomain (url.host(), domain))
                debug_message (url.toString()+" addressed "+domain+" "+QString::number(record->value (domain))+" times. Total "+ResourceClass (type)+" requests from this URL: "+QString::number(record->value ("All")));

        }
    }
    else
    {
        intelligent_resolver_class *group = new intelligent_resolver_class ();;
        intelligent_resolver_record *record = new intelligent_resolver_record ();
        record->insert (domain, 1);
        record->insert ("All",1);
        group->insert (type, record);
        intelligent_resolver->insert (url, group);
        if (!SameUpperDomain (url.host(), domain))
            debug_message (url.toString()+" addressed "+domain+" "+QString::number(record->value (domain))+" times. Total "+ResourceClass (type)+" requests from this URL: "+QString::number(record->value ("All")));

    }
}

// From dooble by Alexis Megas
int RequestFilter::levenshtein_distance (const QString& str1, const QString& str2) const
{
    if (str1.isEmpty())
        return str2.length();
    else if (str2.isEmpty())
        return str1.length();

    QChar str1_c = QChar(0);
    QChar str2_c = QChar(0);
    QVector<QVector<int> > matrix (str1.length() + 1, QVector<int> (str2.length() + 1));
    int cost = 0;

    for (int i = 0; i <= str1.length(); ++i)
        matrix[i][0] = i;

    for (int i = 0; i <= str2.length(); ++i)
        matrix[0][i] = i;

    for (int i = 1; i <= str1.length(); ++i)
    {
        str1_c = str1.at(i - 1);

        for (int j = 1; j <= str2.length(); ++j)
        {
            str2_c = str2.at (j - 1);

            if (str1_c == str2_c)
                cost = 0;
            else
                cost = 1;

            matrix[i][j] = qMin (qMin (matrix[i - 1][j] + 1, matrix[i][j - 1] + 1), matrix[i - 1][j - 1] + cost);
        }
    }

    return matrix[str1.length()][str2.length()];
}
