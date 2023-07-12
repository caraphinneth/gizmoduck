#pragma once
#include <QWebEngineProfile>

#include "webpage.h"

struct TabGroup: public QHash<QString, QSharedPointer<WebPage>>
{

public:
    QWebEngineProfile* profile;

    QStringList order;
    // Insert at position or replace the value for an existing key, keeping position.
    void insert (const QString& key,QSharedPointer<WebPage> const &value, int position = -1)
    {
        if (contains (key))
        {
            // preserve position for now
        }
        // insert at end
        else if (position == -1)
        {
            order.push_back (key);
        }
        else
        {
            order.insert (position+1, key);
        }

        QHash::insert (key, value);
    }

    void remove (const QString& key)
    {
        order.removeOne (key);
        QHash::remove (key);
    }

    void replace (const QString& old_key, const QString& new_key, QSharedPointer<WebPage> const value)
    {
        // URL of existing page changed, preserve position
        order.replace (order.indexOf (old_key), new_key);
        QHash::remove (old_key);
        QHash::insert (new_key, value);
    }

    QSharedPointer<WebPage> assign_page (const QString& key, int position)
    {
        if (contains (key))
        {
            return value (key);
        }
        else
        {
            QSharedPointer<WebPage> p  (new WebPage (profile));
            insert (key, p, position);
            return p;
        }
    }
};

struct TabGroups: public QHash <QString, TabGroup*>
{
public:
    QStringList order;
    void insert (const QString& key, TabGroup* const &value)
    {
        QHash::insert (key, value);
        order.removeOne (key);
        order.push_back (key);
    }
    void remove (const QString& key)
    {
        order.removeOne (key);
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
