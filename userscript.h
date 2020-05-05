#pragma once
#include <QWebEngineScript>

struct UserScript : public QWebEngineScript
{

public:
    UserScript();
    void load_from_file (QString filename);

};
