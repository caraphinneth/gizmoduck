#pragma once
#include <QListView>
#include <QMovie>
#include <QStyledItemDelegate>

#include <webview.h>

struct SideTabs: public QListView
{
    Q_OBJECT

public:
    SideTabs(QWidget* parent = nullptr, int w=162, int h=40);

    // To simplify listview<->stackwidget connection, widget pointers are stored inside a list as Qt::UserRole.
    QWidget* widget (int index);
    int indexOf (QWidget* widget);

signals:
    void temp_pass_wheel_event (QWheelEvent* event);

private:
    void wheelEvent (QWheelEvent* event);
    QIcon loading_icon;
};

class TabHeader: public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TabHeader (QObject* parent = nullptr, int w=162, int h=40, QIcon default_icon=QIcon(QStringLiteral(":/icons/gizmoduck")));

    QSize iconSize;
    QMargins margins;
    int horizontalSpacing;
    int verticalSpacing;
    int width;
    int height;
    QIcon loading_icon;
    void paint (QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint (const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const override;
};
