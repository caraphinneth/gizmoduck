#include <QTreeView>
#include <QVBoxLayout>

#include "debug_tab.h"
#include "message_log.h"

DebugTab::DebugTab (QWidget* parent): QWidget (parent)
{
    QLabel* label1 = new QLabel(tr("Debug messages:"), this);
    MessageLog* debug_view = new MessageLog(this);
    QLabel* label2 = new QLabel(tr("Tab groups:"), this);
    QTreeView* tab_tree = new QTreeView(this);
    QStandardItemModel* model = new QStandardItemModel(this);
    tab_tree->setModel(model);
    QLabel* label3 = new QLabel(tr("History:"), this);
    QListView* history_view = new QListView(this);
    QStandardItemModel* model2 = new QStandardItemModel(this);
    history_view->setModel(model2);

    // debug_view->setMaximumBlockCount (1000);

    QVBoxLayout* layout = new QVBoxLayout (this);
    layout->addWidget(label1);
    layout->addWidget(debug_view);
    layout->addWidget(label2);
    layout->addWidget(tab_tree);
    layout->addWidget(label3);
    layout->addWidget(history_view);
    setLayout(layout);

    connect (this, &DebugTab::message_received, [this, debug_view] (const QString& text)
    {
        debug_view->append (text, ":/icons/system", QDateTime::currentDateTime());
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
                QStandardItem* child = new QStandardItem(j.key());
                item->appendRow (child);
            }
        }
        tab_tree->expandAll();
    });

    connect (this, &DebugTab::redraw_history, [this, model2, history_view](const History& history)
    {
        model2->clear();
        foreach(const QUrl& s, history)
        {
            QStandardItem* item = new QStandardItem(s.toString());
            model2->appendRow(item);
        }
    });
}
