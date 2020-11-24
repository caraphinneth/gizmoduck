#pragma once
#include "gl_widget.h"

struct SettingsTab: public GLWidget
{
    Q_OBJECT

public:
    explicit SettingsTab (QWidget* parent = nullptr);

signals:
    void reload_filters();
};