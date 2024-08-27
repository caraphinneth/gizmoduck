#pragma once
#include "tab_groups.h"

struct DebugTab: public QWidget
{
    Q_OBJECT

public:
    explicit DebugTab(QWidget* parent = nullptr);

signals:
    void message_received(const QString& text);
    void redraw_tabs(const TabGroups& groups);
    void redraw_history(const History& history);
};
