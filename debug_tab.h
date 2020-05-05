#pragma once
#include "gl_widget.h"

struct DebugTab : public GLWidget
{
    Q_OBJECT

public:
    explicit DebugTab (QWidget *parent = nullptr);
    QPlainTextEdit *debug_view;

public slots:
    void message (const QString &text);

signals:
    void message_received (const QString &text);
};


