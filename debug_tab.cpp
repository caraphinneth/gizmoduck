#include "debug_tab.h"
#include "message_log.h"

DebugTab::DebugTab (QWidget *parent): GLWidget (parent)
{
    QLabel *label = new QLabel (tr("Debug messages:"), this);
    MessageLog *debug_view = new MessageLog(this); //QPlainTextEdit ("",this);

    // debug_view->setMaximumBlockCount (1000);

    QVBoxLayout *layout = new QVBoxLayout (this);
    layout->addWidget (label);
    layout->addWidget (debug_view);
    setLayout(layout);

    // Widgets aren't thread-safe so you cannot call their methods without connecting.
    connect (this, &DebugTab::message_received, [this, debug_view] (const QString &text)
    {
        //const bool atBottom = debug_view->verticalScrollBar()->value() == debug_view->verticalScrollBar()->maximum();

        debug_view->append (text, QPixmap (QStringLiteral (":/icons/system")), QDateTime::currentDateTime());
        // if (atBottom) debug_view->verticalScrollBar()->setValue (debug_view->verticalScrollBar()->maximum());
    });
}

void DebugTab::message (const QString &text)
{
    emit message_received (text);
}
