#include <QFile>
#include <QDebug>

#include "userscript.h"

UserScript::UserScript(): QWebEngineScript()
{
    setRunsOnSubFrames (true);
    setWorldId (QWebEngineScript::ApplicationWorld);
}

void UserScript::load_from_file (const QString& filename)
{
    qDebug() << "Loading script from file:"  << filename;

    QFile file (filename);
    file.open (QIODevice::ReadOnly | QIODevice::Text);
    QString source = file.readAll();
    file.close();

    setName (filename);
    setSourceCode (source);
}
