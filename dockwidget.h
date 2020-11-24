#pragma once
#include <QDockWidget>


class DockWidget: public QDockWidget
{
    Q_OBJECT

public:
    DockWidget (const QString& title = QString(), QWidget* parent = nullptr);

protected:
    void closeEvent (QCloseEvent* event);

signals:
    void closed();
};