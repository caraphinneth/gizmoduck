//Pretty much just QOpenGLWidget, used as base class for better performance during high GPU load.

#pragma once
#include <QtWidgets>

struct GLWidget: public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GLWidget (QWidget* parent = nullptr);

protected:
    void initializeGL();
};

