#pragma once
#include <QWidget>
//#include "gl_widget.h"

struct SettingsTab: public QWidget
{
    Q_OBJECT

public:
    explicit SettingsTab (QWidget* parent = nullptr);

signals:
    void reload_filters();
    void avatar_changed();
    void name_update (const QString& name);
    void status_update (const QString& status);
};