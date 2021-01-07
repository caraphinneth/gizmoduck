#pragma once
#include <QWebEngineProfile>

#include "webpage.h"

struct TabGroup: public QHash<QString, WebPage*>
{

public:
    QWebEngineProfile* profile;

    std::list<QString> order;
    void insert (const QString& key, WebPage* const &value, std::list<QString>::iterator position)
    {
        if (contains (key))
        {
            // preserve position for now
        }
        else if (position == order.end())
        {
            order.push_back (key);
        }
        else
        {
            order.insert (position, key);
        }

        QHash::insert (key, value);
    }

    void remove (const QString& key)
    {
        order.remove (key);
        QHash::remove (key);
    }

    void replace (const QString& old_key, const QString& new_key, WebPage* const value)
    {
        // URL of existing page changed, preserve position
        auto it = std::find (order.begin(), order.end(), old_key);
        if (it!=order.end())
        {
            order.insert (std::next(it), new_key);
            order.erase (it);
        }
        QHash::remove (old_key);
        QHash::insert (new_key, value);
    }

    WebPage* assign_page (const QString& key, std::list<QString>::iterator position)
    {
        if (contains (key))
        {
            return value (key);
        }
        else
        {
            WebPage* p (new WebPage (profile));
            insert (key, p, position);
            return p;
        }
    }
};

struct TabGroups: public QHash <QString, TabGroup*>
{
public:
    std::list<QString> order;
    void insert (const QString& key, TabGroup* const &value)
    {
        QHash::insert (key, value);
        order.remove (key);
        order.push_back (key);
    }
    void remove (const QString& key)
    {
        order.remove (key);
        QHash::remove (key);
    }
};

class History: public std::list<QUrl>
{
public:
    std::list<QUrl>::iterator current;

    History() : std::list<QUrl>()
    {
        current = begin();
    }

    void add (const QUrl& url)
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
