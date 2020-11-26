#include "debug_tab.h"
#include "message_log.h"

DebugTab::DebugTab (QWidget* parent): QWidget (parent)
{
    QLabel* label1 = new QLabel (tr("Debug messages:"), this);
    MessageLog* debug_view = new MessageLog(this);
    QLabel* label2 = new QLabel (tr("Tab groups:"), this);
    QTreeView* tab_tree = new QTreeView (this);
    QStandardItemModel* model = new QStandardItemModel (this);
    tab_tree->setModel(model);

    // debug_view->setMaximumBlockCount (1000);

    QVBoxLayout *layout = new QVBoxLayout (this);
    layout->addWidget (label1);
    layout->addWidget (debug_view);
    layout->addWidget (label2);
    layout->addWidget (tab_tree);
    setLayout(layout);

    connect (this, &DebugTab::message_received, [this, debug_view] (const QString& text)
    {
        //const bool atBottom = debug_view->verticalScrollBar()->value() == debug_view->verticalScrollBar()->maximum();

        debug_view->append (text, QPixmap (QStringLiteral (":/icons/system")), QDateTime::currentDateTime());
        // if (atBottom) debug_view->verticalScrollBar()->setValue (debug_view->verticalScrollBar()->maximum());
    });

    connect (this, &DebugTab::redraw_tabs, [this, model, tab_tree](const TabGroups& groups)
    {
        model->clear();
        for (auto i = groups.begin(); i != groups.end(); ++i)
        {
            QStandardItem* item = new QStandardItem (i.key());
            model->appendRow (item);
            TabGroup* group = i.value();
            for (auto j = group->begin(); j != group->end(); ++j)
            {
                QStandardItem* child = new QStandardItem (j.key());
                item->appendRow (child);
            }
        }
        tab_tree->expandAll();
    });
}