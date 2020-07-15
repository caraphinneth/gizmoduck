#pragma once
#include "webpage.h"

struct TabGroup: public QHash <QString, WebPage*>
{

};

struct TabGroups: public QHash <QString, TabGroup*>
{

};