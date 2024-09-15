#include <QLabel>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include "process_tab.h"

ProcessTab::ProcessTab(QWidget* parent): QWidget(parent)
{
    QLabel* label(new QLabel(tr("Running processes:"), this));
    process_list = new QListView(this);
    model = new QStandardItemModel(this);
    process_list->setModel(model);
    QStyledItemDelegate* delegate = new ProcessInfo(process_list);
    process_list->setItemDelegate(delegate);
    QVBoxLayout* layout(new QVBoxLayout(this));
    layout->addWidget(label);
    layout->addWidget(process_list);
    setLayout(layout);
}

void ProcessTab::add_process(const QString& process_name)
{
    QStandardItem* item = new QStandardItem(QIcon(":/icons/download"), QString("<b>" + process_name + "</b><br>Starting..."));
    model->appendRow(item);
}

void ProcessTab::update_process(const QString& process_name, const QByteArray& output)
{
    if (auto item = model->findItems("<b>" + process_name + "</b>", Qt::MatchStartsWith).first())
    {
        item->setData(QString("<b>" + process_name + "</b><br>" + QString::fromUtf8(output)), Qt::DisplayRole);
    }
    else
    {
        qDebug() << "BUG: feedback from an unlisted process:" << process_name;
    }
}

void ProcessTab::finish_process(const QString& process_name, int status)
{
    if (auto item = model->findItems("<b>" + process_name + "</b>", Qt::MatchStartsWith).first())
    {
        item->setData(QIcon(status ? ":/icons/alert" : ":/icons/ok"), Qt::DecorationRole);
    }
    else
    {
        qDebug() << "BUG: feedback from an unlisted process:" << process_name;
    }
}
