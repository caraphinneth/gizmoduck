// Custom line edit, intercepting image pasting.

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>

#include "input_widget.h"

InputWidget::InputWidget (QWidget* parent) : QLineEdit (parent)
{

}

// ported from qTox, but it needs improvement
bool InputWidget::check_for_image()
{
    const QClipboard* const clipboard = QApplication::clipboard();
    if (!clipboard)
    {
        return false;
    }

    /*const QMimeData* const mimeData = clipboard->mimeData();
    if (!mimeData || !mimeData->hasImage())
    {
        return false;
    }
    */

    const QPixmap pixmap (clipboard->pixmap());
    if (pixmap.isNull())
    {
        return false;
    }

    emit paste_image (pixmap);
    return true;
}

void InputWidget::keyPressEvent (QKeyEvent* event)
{
    if (event->matches (QKeySequence::Paste) && check_for_image())
    {
        return;
    }
    QLineEdit::keyPressEvent (event);
}