// Adapted from Michael Scopchanov's example.

#include <QDebug>
#include <QDateTime>
#include <QScrollBar>
#include <QPainter>

#include "message_log.h"

MessageLog::MessageLog (QWidget* parent): QListView (parent)
{
    setVerticalScrollMode (QAbstractItemView::ScrollPerPixel);

    auto* message = new Message(this);

    QPalette p(palette());

    p.setBrush(QPalette::WindowText, QColor("#303030"));
    //p.setBrush(QPalette::Base, QColor("#F0F1F2"));
    p.setBrush(QPalette::Light, QColor("#FFFFFF"));
    p.setBrush(QPalette::Midlight, QColor("#D3D6D8"));
    p.setBrush(QPalette::Mid, QColor("#C5C9Cb"));
    p.setBrush(QPalette::Dark, QColor("#9AA0A4"));
    p.setBrush(QPalette::Text, QColor("#616b71"));
    p.setBrush(QPalette::Highlight, QColor("#E2E4E5"));

    message->margins = QMargins (8, 8, 8, 8);
    message->iconSize = QSize (48, 48);
    message->horizontalSpacing = 8;
    message->verticalSpacing = 4;

    setPalette(p);
    setFont (QFont ("Roboto", 12));

    // setWordWrap (true);
    // setResizeMode (QListView::Adjust);
    //setSizeAdjustPolicy (QAbstractScrollArea::AdjustToContents);
    setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

    setModel (new CachedModel(this));
    setItemDelegate (message);
    setEditTriggers (QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked);
}

void MessageLog::append (const QString &text, const QPixmap &pixmap, const QDateTime &dateTime, bool file)
{
    auto* item = new QStandardItem (QIcon(pixmap), text);

    item->setData (dateTime.toString("yyyy-MM-dd HH:mm:ss"), Qt::UserRole);

    if (!file)
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    else
    {
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setData (text, Qt::UserRole+1);
    }

    const bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();

    // setContentsMargins (QMargins (8, 8, 8, 8));

    static_cast<QStandardItemModel*>(model())->appendRow(item);

    if (atBottom)
        scrollToBottom();
}

void MessageLog::clear()
{
    static_cast<QStandardItemModel*>(model())->clear();
}

Message::Message (QObject* parent) : QStyledItemDelegate(parent)
{
    iconSize = QSize (48, 48);
    margins = QMargins (0, 0, 0, 0);
    horizontalSpacing = 0;
    verticalSpacing = 0;
}

void MessageLog::resizeEvent (QResizeEvent* event)
{
    scheduleDelayedItemsLayout();
    QAbstractItemView::resizeEvent (event);
}

void Message::paint (QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt (option);
    initStyleOption (&opt, index);

    const QPalette &palette (opt.palette);
    const QRect &rect (opt.rect);
    const QRect &contentRect (rect.adjusted (margins.left(), margins.top(), -margins.right(), -margins.bottom()));
    const bool lastIndex = (index.model()->rowCount() - 1) == index.row();
    const bool hasIcon = !opt.icon.isNull();
    const int bottomEdge = rect.bottom();
    QFont f (opt.font);

    f.setPointSize (timestampFontPointSize (opt.font));

    painter->save();
    painter->setClipping(true);
    painter->setClipRect(rect);
    painter->setFont(opt.font);

    // Draw background
    painter->fillRect(rect, opt.state & QStyle::State_Selected ?
                          palette.highlight().color() :
                          palette.light().color());

    // Draw bottom line
    painter->setPen(lastIndex ? palette.dark().color()
                              : palette.mid().color());
    painter->drawLine(lastIndex ? rect.left() : margins.left(),
                      bottomEdge, rect.right(), bottomEdge);

    // Draw message icon
    if (hasIcon)
        painter->drawPixmap(contentRect.left(), contentRect.top(),  opt.icon.pixmap(iconSize));

    // Draw timestamp
    QRect timeStampRect (timestampBox(opt, index));

    timeStampRect.moveTo (margins.left() + iconSize.width() + horizontalSpacing, contentRect.top());

    painter->setFont (f);
    painter->setPen (palette.text().color());
    painter->drawText (timeStampRect, Qt::TextSingleLine, index.data (Qt::UserRole).toString());

    // Draw message text

    QScopedPointer <QTextDocument> text;
    text.reset (messageBox (opt, index));
    QRect messageRect (0, 0, text->size().width(), text->size().height());
    messageRect.moveTo (timeStampRect.left(), timeStampRect.bottom() + verticalSpacing);

    painter->setFont (opt.font);
    painter->setPen (palette.windowText().color());

    painter->translate (QPoint (messageRect.left(), messageRect.top()));
    text->drawContents (painter, QRect (0, 0, text->size().width(), text->size().height()));
    //text.idealWidth(), textDoc.size().height()
    //painter->drawText(messageRect, Qt::TextWordWrap, opt.text);

    painter->restore();
}

QSize Message::sizeHint (const QStyleOptionViewItem& option,  const QModelIndex& index) const
{
    QStyleOptionViewItem opt (option);
    initStyleOption (&opt, index);

    QScopedPointer <QTextDocument> text;
    text.reset (messageBox (opt, index, true));
    int textHeight = timestampBox (opt, index).height() + verticalSpacing + text->size().height();
    int iconHeight = iconSize.height();
    int h = textHeight > iconHeight ? textHeight : iconHeight;

    // printf ("sizehint height %i\n", h);
    return QSize (opt.rect.width(), margins.top() + h + margins.bottom() + 8);
}

QRect Message::timestampBox(const QStyleOptionViewItem& option,  const QModelIndex& index) const
{
    QFont f (option.font);

    f.setPointSizeF(timestampFontPointSize(option.font));

    return QFontMetrics(f).boundingRect(index.data(Qt::UserRole).toString()).adjusted(0, 0, 5, 1);
}

qreal Message::timestampFontPointSize (const QFont &f) const
{
    return 0.85*f.pointSize();
}

QTextDocument* Message::messageBox (const QStyleOptionViewItem& option, const QModelIndex& index, bool fastmode) const
{
    QStyleOptionViewItem opt (option);
    initStyleOption (&opt, index);
    QTextDocument* text = new QTextDocument;

    int good_width = opt.rect.width() - margins.left() - margins.right() - iconSize.width() - horizontalSpacing;
    /*QTextOption t_option;
    t_option.setWrapMode (QTextOption::WordWrap);
    text->setDefaultTextOption (t_option);*/
    text->setDefaultFont (opt.font);
    text->setTextWidth (good_width);
    text->setDocumentMargin (0);

    if (!index.data (Qt::UserRole+1).isNull ())
    {
        QTextCursor cursor (text);
        cursor.movePosition (QTextCursor::Start);

        QImage img (opt.text);
        Qt::TransformationMode mode = fastmode ? Qt::FastTransformation : Qt::SmoothTransformation;

        if (img.isNull())
        {
            QString info_text = "File sent: "+opt.text;
            cursor.insertText (info_text);
        }
        else
            cursor.insertImage (img.scaledToWidth (good_width, mode));
    }
    else
    {
        text->setMarkdown (opt.text);
    }
    return text;
}

QWidget* Message::createEditor (QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    MessageEditor* editor = new MessageEditor (parent);
    return editor;
}

void Message::updateEditorGeometry (QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt (option);
    initStyleOption (&opt, index);
    const QRect &rect (opt.rect);
    const QRect &contentRect (rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom()));
    QRect timeStampRect(timestampBox(opt, index));
    timeStampRect.moveTo (margins.left() + iconSize.width() + horizontalSpacing, contentRect.top());
    QScopedPointer <QTextDocument> text;
    text.reset (messageBox (opt, index, true));
    QRect messageRect (0, 0, text->size().width(), text->size().height());
    messageRect.moveTo (timeStampRect.left(), timeStampRect.bottom() + verticalSpacing);
    editor->setGeometry (messageRect);
}

void Message::setEditorData (QWidget* editor, const QModelIndex& index) const
{
    MessageEditor* ed = qobject_cast<MessageEditor *>(editor);

    QPalette p (editor->palette());

    p.setBrush (QPalette::Highlight, QColor(Qt::darkBlue));

    editor->setPalette (p);
    editor->setFont (QFont("Roboto", 12));
    ed->setTextFormat (Qt::MarkdownText);

    ed->setText (qvariant_cast<QString> (index.data()));
    ed->setTextInteractionFlags (Qt::TextBrowserInteraction);
    ed->setWordWrap (true);
}

MessageEditor::MessageEditor (QWidget* parent): QLabel (parent)
{

}

CachedModel::CachedModel (QObject* parent): QStandardItemModel (parent), cache (10)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName (":memory:");
    if (!db.open())
    {
        qDebug() << "Unable to create an sqlite connection!";
        return;
    }

    QSqlQuery query (db);
    query.exec ("create table test (id int primary key, time int not null, friend_id int, message varchar(200))");

    query.exec ("insert into test values (1, 1, 1, 'Test test test')");

    query.exec ("insert into test values (2, 2, 4, 'Should not see me!')");

    query.exec ("select * from test where friend_id is 1");
    while (query.next())
    {
        QString country = query.value (3).toString();
        qDebug() << "Query result" <<country;
    }
    db.close();
}

QVariant CachedModel::data (const QModelIndex& _index, int role) const
{    
    const int lookAhead (2);
    const int halfLookAhead (lookAhead/2);

    int row = _index.row();

    if (row > cache.lastIndex()) {
        if (row - cache.lastIndex() > lookAhead)
            cacheRows (row-halfLookAhead, qMin (rowCount(), row+halfLookAhead));
        else while (row > cache.lastIndex())
            cache.append (fetchRow (cache.lastIndex()+1));
    } else if (row < cache.firstIndex()) {
        if (cache.firstIndex() - row > lookAhead)
            cacheRows (qMax (0, row-halfLookAhead), row+halfLookAhead);
        else while (row < cache.firstIndex())
            cache.prepend (fetchRow (cache.lastIndex()-1));
    }

    return cache.at(row).value (role);
}

void CachedModel::cacheRows (int from, int to) const
{
    for (int i = from; i <= to; ++i)
        cache.insert (i, itemData (index (i, 0)));
}

QMap<int, QVariant> CachedModel::fetchRow (int position) const
{
    return itemData (index (position, 0));
}