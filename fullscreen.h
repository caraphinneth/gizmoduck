#pragma once

#include "gl_widget.h"
#include "QWebEngineView"

class FullScreenWindow : public GLWidget
{
    Q_OBJECT

public:
    explicit FullScreenWindow (QWebEngineView* _old_view, QWidget* parent = nullptr);
    ~FullScreenWindow();

protected:
    void resizeEvent (QResizeEvent* event) override;

private:
    QWebEngineView* view;
    QWebEngineView* old_view;
};