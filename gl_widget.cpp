#include "gl_widget.h"

// Basic GL widget to work around Qt weird rasterizing system.
GLWidget::GLWidget (QWidget* parent): QOpenGLWidget (parent, Qt::Widget)
{

}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    QColor widget_color = QApplication::palette ("QWidget").color (QPalette::Window);
    glClearColor (widget_color.redF(), widget_color.greenF(), widget_color.blueF(), widget_color.alphaF());
}
