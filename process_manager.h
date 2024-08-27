#pragma once
#include <QHash>
#include <QProcess>
#include <QSharedPointer>

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    ProcessManager(QObject* parent = nullptr);
    void start_process(const QString& executable, const QStringList& arguments);

signals:
    void started(const QString& process_name);
    void output_received(const QString& process_name, const QByteArray& output);
    void finished(const QString& process_name, int code);
};
