#pragma once
#include <QLineEdit>


class InputWidget : public QLineEdit
{
    Q_OBJECT

public:
    InputWidget (QWidget* parent = nullptr);
private:
    bool check_for_image();
protected:
    void keyPressEvent (QKeyEvent* event);

signals:
    void paste_image (const QPixmap& pixmap);
};