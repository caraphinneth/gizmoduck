#include <QApplication>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLibraryInfo>
#include <QSettings>
#include <QTranslator>

#include "browser_mainwindow.h"

QString url_argument()
{
    const QStringList args = QCoreApplication::arguments();
    for (const QString& arg : args.mid(1))
    {
        if (!arg.startsWith(u'-'))
        {
            return arg;
        }
    }
    return "about:blank";
}

void debug_handler (QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char* file = context.file ? context.file : "";
    const char* function = context.function ? context.function : "";
    FILE* f = fopen ("debug.log", "a");
    switch (type)
    {
    case QtDebugMsg:
        fprintf (f, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf (f, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf (f, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf (f, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf (f, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
    fclose (f);
}

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute (Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setApplicationName ("Gizmoduck");
    // Windows will fail to write settings without organization.
#ifdef WIN32
    QCoreApplication::setOrganizationName ("Clockwork Fairies");
#endif
    //setWindowTitle( QCoreApplication::applicationName() );

#ifdef QT_DEBUG
    const QString localServerName = "Gizmoduck debug listener";
    const QString uniqueKey = "Gizmoduck debug";
#else
    const QString localServerName = "Gizmoduck listener";
    const QString uniqueKey = "Gizmoduck";
#endif


    // EXPERT SETTINGS ARE NO LONGER ACCEPTED BY COMMANDLINE.
    // Use QTWEBENGINE_CHROMIUM_FLAGS env variable instead.

    // qInstallMessageHandler (debug_handler);

    QApplication app (argc, argv);
    app.setWindowIcon (QIcon (QStringLiteral (":/icons/gizmoduck")));

    QTranslator qt_translator;
    qt_translator.load ("qt_" + QLocale::system().name(), QLibraryInfo::location (QLibraryInfo::TranslationsPath));
    app.installTranslator (&qt_translator);

    QTranslator webengine_translator;
    webengine_translator.load ("qtwebengine_" + QLocale::system().name(), QLibraryInfo::location (QLibraryInfo::TranslationsPath));
    app.installTranslator (&webengine_translator);

    QTranslator browser_translator;
    browser_translator.load (":/locale/gizmoduck_" + QLocale::system().name());
    app.installTranslator (&browser_translator);

    QSharedMemory sharedMemory(uniqueKey);
    if (sharedMemory.attach())
    {
        QLocalSocket socket;
        socket.connectToServer(localServerName);

        if (socket.waitForConnected())
        {
            socket.write(url_argument().toUtf8());
            socket.flush();
            socket.disconnectFromServer();
        }
        return 0;
    }

    sharedMemory.create(4096);

    QLocalServer server;
    server.listen(localServerName);

    MainWindow main_window;
    main_window.show();

    QObject::connect(&server, &QLocalServer::newConnection, [&]()
    {
        QLocalSocket* clientConnection = server.nextPendingConnection();
        QObject::connect(clientConnection, &QLocalSocket::readyRead, [&]()
        {
            QByteArray data = clientConnection->readAll();
            QUrl url = QUrl::fromUserInput(data);
            main_window.open_url(url);
        });
    });

    return app.exec();
}
