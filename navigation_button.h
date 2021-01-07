// Buttons with custom left/right click handling.

#pragma once
#include <QPushButton>

struct NavButton: public QPushButton
{
    Q_OBJECT

public:
    NavButton (QWidget* parent = nullptr);

signals:
    void left_clicked();
    void mid_clicked();
    void right_clicked();

protected:
     void mousePressEvent (QMouseEvent* event) override;
};


