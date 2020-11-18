#pragma once
#include <QWebEngineProfile>
#include "webpage.h"

struct TabGroup: public QHash<QString, QSharedPointer<WebPage>>
{

public:
    QWebEngineProfile* profile;

    QSharedPointer<WebPage> assign_page (QString key)
    {
        if (contains (key))
        {
            return value (key);
        }
        else
        {
            QSharedPointer<WebPage> p;
            p.reset (new WebPage(profile));
            insert (key, p);
            return p;
        }
    }
};

struct TabGroups: public QHash <QString, TabGroup*>
{

};

class History: public std::list<QUrl>
{
public:
    std::list<QUrl>::iterator current;

    History() : std::list<QUrl>()
    {
        current = begin();
    }

    void add (QUrl url)
    {
        if (url.isEmpty())
            return;
        if (current == end())
        {
            push_back (url);
            current = begin();
        }
        else
            current = insert (std::next(current), url);

        qDebug()<< "History now points at" << current->toString();
    }
    void forward()
    {
        if (current!=std::prev(end()))
            ++current;
    }
    void back()
    {
        if (current!=begin())
            --current;
        qDebug()<< "History now points at" << current->toString();       
    }

};