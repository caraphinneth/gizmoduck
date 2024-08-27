#pragma once
#include <QListView>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTextDocument>

struct ProcessTab: public QWidget
{
    Q_OBJECT

public:
    explicit ProcessTab(QWidget* parent = nullptr);

public slots:
    void add_process(const QString& process_name);
    void update_process(const QString& process_name, const QByteArray& output);
    void finish_process(const QString& process_name, int status);

private:
    QListView* process_list;
    QStandardItemModel* model;
};

struct ProcessInfo: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ProcessInfo(QObject *parent = nullptr) : QStyledItemDelegate(parent)
    {

    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QString htmlText = index.data(Qt::DisplayRole).toString();

        QTextDocument doc;
        doc.setHtml(htmlText);
        doc.setTextWidth(option.rect.width());

        painter->save();
        painter->setClipRect(option.rect);
        doc.drawContents(painter, option.rect);
        painter->restore();
    }
};
