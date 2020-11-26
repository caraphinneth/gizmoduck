#include "fullscreen.h"

FullScreenWindow::FullScreenWindow (QWebEngineView* _old_view, QWidget* parent): GLWidget (parent)
    {
    view = new QWebEngineView(this);
    old_view = _old_view;
    QAction* exit_action = new QAction (this);
    exit_action->setShortcut (Qt::Key_Escape);
    connect (exit_action, &QAction::triggered, [this]()
    {
        view->triggerPageAction (QWebEnginePage::ExitFullScreen);
    });

    addAction (exit_action);

    view->setPage(old_view->page());
    setGeometry (old_view->window()->geometry());
    showFullScreen();
    //old_view->window()->hide();
}

FullScreenWindow::~FullScreenWindow()
{
    old_view->setPage (view->page());
    hide();
}

void FullScreenWindow::resizeEvent (QResizeEvent* event)
{
    QRect viewGeometry (QPoint(0, 0), size());
    view->setGeometry (viewGeometry);
    QWidget::resizeEvent(event);
}