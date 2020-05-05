#pragma once
#include <QTabBar>
#include <QMovie>

struct SideTabs : public QTabBar
{
    Q_OBJECT

public:
    SideTabs (QWidget *parent = nullptr, int w=256, int h=40);
    QSize tabSizeHint (int index);

private:
    void paintEvent (QPaintEvent*);
    QMovie *loading_icon;

};

