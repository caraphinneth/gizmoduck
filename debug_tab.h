#pragma once
#include "gl_widget.h"
#include "tab_groups.h"

struct DebugTab: public GLWidget
{
    Q_OBJECT

public:
    explicit DebugTab (QWidget* parent = nullptr);

signals:
    void message_received (const QString& text);
    void redraw_tabs (const TabGroups& groups);
};