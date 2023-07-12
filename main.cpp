#include <QApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QTranslator>

#include "browser_mainwindow.h"

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

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute (Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setApplicationName ("Gizmoduck");
    // Windows will fail to write settings without organization.
#ifdef WIN32
    QCoreApplication::setOrganizationName ("Clockwork Fairies");
#endif
    //setWindowTitle( QCoreApplication::applicationName() );

    QStringList arguments;

    for (int i=0; i<argc; ++i)
    {
        arguments.append (QString (argv[i]));
    }

    QSettings settings;

    settings.beginGroup ("ProcessModel");
    switch (settings.value ("type", 1).toInt())
    {
        case 1: arguments.append ("--process-per-site"); break;
        case 2: arguments.append ("--single-process"); break;
        //default: break;
    }
    settings.endGroup();

    settings.beginGroup ("Web settings");
    if (settings.value ("tcp_fast_open", false).toBool())
        arguments.append ("--enable-tcp-fastopen");
    else
        arguments.append ("--disable-tcp-fastopen");
    if (settings.value ("checker_imaging", false).toBool())
        arguments.append ("--enable-checker-imaging");
    else
        arguments.append ("--disable-checker-imaging");
    settings.endGroup();



    settings.beginGroup ("Rasterizer");
    if (settings.value ("gpu_enabled", false).toBool())
        arguments.append ("--enable-gpu-rasterization");
    else
        arguments.append ("--disable-gpu-rasterization");
    /*
    if (settings.value ("zero_copy", false).toBool())
        arguments.append ("--enable-zero-copy");
    else
        arguments.append ("--disable-zero-copy");
    */
    settings.endGroup();

    settings.beginGroup ("Experimental");
    if (settings.value ("ignore_gpu_blacklist", false).toBool())
        arguments.append ("--ignore-gpu-blacklist");
    /*
    if (settings.value ("enable_gpu_buffers", false).toBool())
        arguments.append ("--enable-native-gpu-memory-buffers");
    else
        arguments.append ("--disable-native-gpu-memory-buffers");
    */
    settings.endGroup();

    //arguments.append ("--use-vulkan");
    //arguments.append ("--webview-enable-vulkan");
    //arguments.append ("--enable-features=Vulkan");
    //arguments.append ("--allow-file-access-from-files");
    //#arguments.append ("--enable-oop-rasterization");
    //arguments.append ("--use-gl=egl");

    argc = arguments.count();

    char** newargv = new char* [arguments.count()];
    for (int i=0; i<arguments.count(); ++i)
    {
        newargv[i] = new char[arguments[i].toLocal8Bit().size()+1]; // This dies fast so yeah...
        strcpy (newargv[i], arguments[i].toLocal8Bit().data());
    }    

    qInstallMessageHandler (debug_handler);

    QApplication app (argc, newargv);
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

    MainWindow main_window;
    main_window.show();

    return app.exec();
}
