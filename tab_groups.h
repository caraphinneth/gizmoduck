#pragma once
#include "webpage.h"

struct TabGroup: public QHash <QString, WebPage*>
{

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
        current = end();
    }

    void add (QUrl url)
    {
        if (current!=end())
            current = insert (std::next(current), url);
        else
        {
            push_back (url);
            ++current;
        }
    }
    void forward()
    {
        if (current!=end())
            ++current;
    }
    void back()
    {
        if (current!=begin())
            --current;
        qDebug()<< "History now points at" << current->toString();
    }

};
