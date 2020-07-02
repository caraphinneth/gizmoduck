#include "QMouseEvent"

#include "navigation_button.h"

// Custom buttons reacting to various sorts of clicks differently.
NavButton::NavButton (QWidget *parent): QPushButton (parent)
{
    setFlat (true);
}

void NavButton::mousePressEvent (QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit left_clicked();
    }
    else if (event->button() == Qt::MidButton)
    {
        emit mid_clicked();
    }
    else if (event->button() == Qt::RightButton)
    {
        emit right_clicked();
    }

    QPushButton::mousePressEvent(event);
}
