#pragma once
#include <QWebEngineProfile>

#include "webpage.h"

struct TabGroup: public QHash<QString, QSharedPointer<WebPage>>
{

public:
    QWebEngineProfile* profile;

    QStringList order;
    // Insert at position or replace the value for an existing key, keeping position.
    void insert(const QString& key, QSharedPointer<WebPage> const &value, int position=-1)
    {
        if (contains(key))
        {
            // preserve position for now
        }
        // insert at end
        else
        {
            if(position == -1)
            {
                order.push_back(key);
            }
            else
            {
                order.insert(position+1, key);
            }
            QHash::insert(key, value);
        }
    }

    void remove(const QString& key)
    {
        order.removeAll(key);
        QHash::remove(key);
    }

    void replace(const QString& old_key, const QString& new_key, QSharedPointer<WebPage> const value)
    {
        // URL of existing page changed, preserve position
        if (order.contains(old_key))
        {
            order.replace(order.indexOf(old_key), new_key);
        }
        QHash::insert(new_key, value);
        QHash::remove(old_key);
    }

    QWeakPointer<WebPage> get_page(const QString& key, int position=-2)
    // If no position is provided, works as getter.
    // If position specified, works as getter if the key is present.
    // Otherwise, inserts a new page into the hashtable.
    {
        if (contains(key))
        {
            if (position != -2)
            {
                qDebug() << "WARNING: the page is supposed to be new, but already exists:" << key;
            }
            return QWeakPointer<WebPage>(value(key));
        }
        else if (position != -2)
        {
            QSharedPointer<WebPage> p(new WebPage(profile));
            insert(key, p, position);
            return QWeakPointer<WebPage>(p);
        }
        else
        {
            qDebug() << "BUG: a page does not exist in this group!" << key;
            return QWeakPointer<WebPage>();
        }

    }
};

struct TabGroups: public QHash<QString, TabGroup*>
{
public:
    QStringList order;
    void insert(const QString& key, TabGroup* const &value)
    {
        QHash::insert(key, value);
        order.removeAll(key);
        order.push_back(key);
    }
    void remove(const QString& key)
    {
        order.removeAll(key);
        QHash::remove(key);
    }
};

class History : public QList<QUrl>
{
public:
    QList<QUrl>::iterator current;

    History() : QList<QUrl>()
    {
        current = begin();
    }

    void add(const QUrl& url)
    {
        if (url.isEmpty())
            return;
        if (current == end())
        {
            push_back(url);
            current = begin();
        }
        else
            current = insert(std::next(current), url);
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
    }
};
