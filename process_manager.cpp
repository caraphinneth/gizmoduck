#include <QDebug>
#include "process_manager.h"


ProcessManager::ProcessManager(QObject* parent): QObject(parent)
{

}

void ProcessManager::start_process(const QString& executable, const QStringList& arguments)
{
    QSharedPointer<QProcess> process(new QProcess(this));
    QString process_name = executable + ' ' + arguments.last();

    connect(process.data(), &QProcess::started, this, [this, process_name]()
    {
        emit started(process_name);
    });

    connect(process.data(), &QProcess::readyRead, this, [this, process, process_name]()
    {
        while (process->canReadLine())
        {
            QByteArray output = process->readLine();
            emit output_received(process_name, output);
        }
     });

    connect(process.data(), &QProcess::finished, this, [this, process, process_name](int code, QProcess::ExitStatus /*status*/)
    {
        emit finished(process_name, code);
    });

    process->start(executable, arguments);
}
