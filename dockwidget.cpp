#include "dockwidget.h"

DockWidget::DockWidget (const QString& title, QWidget* parent) : QDockWidget (title, parent)
{

}

void DockWidget::closeEvent (QCloseEvent* event)
{
    emit closed();
    QDockWidget::closeEvent(event);
}