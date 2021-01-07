// Adapted from Michael Scopchanov's example.

#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QSqlQuery>
#include <QTimer>

#include "message_log.h"

MessageLog::MessageLog (QWidget* parent, const QString& _name): QTableView (parent)
{
    setVerticalScrollMode (QAbstractItemView::ScrollPerPixel);

    auto* message = new Message (this);

    QPalette p (palette());

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

    name = _name;
    //setUniformItemSizes (true);

    setShowGrid (false);
    horizontalHeader()->setSectionResizeMode (QHeaderView::Stretch);
    horizontalHeader()->setVisible(false);
    //verticalHeader ()->setDefaultSectionSize(64);
    verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    verticalHeader()->setVisible(false);
    setSortingEnabled (false);

    setModel (new CachedModel (this, name));
    setItemDelegate (message);

    setEditTriggers (QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked);

    //qDebug()<<"Constructor model size:"<<model()->rowCount();
    scrollToBottom();

    firstIndex = -1;
    lastIndex = -1;

    connect (static_cast<CachedModel*>(model()), &CachedModel::window_changed, [this](const int first, const int last)
    {
        const bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();

        for (int i = first; i<=last;++i)
        {
            resizeRowToContents(i);
            //qDebug()<<"Resizing row"<<i;
        }

        firstIndex = first;
        lastIndex = last;

        if (atBottom)
            QTimer::singleShot (1, this, &QTableView::scrollToBottom);

    });
}

void MessageLog::index_search (const QString& text)
{
    const bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();
    CachedModel* _model = static_cast<CachedModel*>(model());
    search_index = _model->search (text);
    if (atBottom)
        QTimer::singleShot (1, this, &MessageLog::search_back);
}

void MessageLog::search_back()
{
    if (!search_index.isEmpty())
    {
        search_index.prepend (search_index.takeLast());
        scrollTo (model()->index (search_index.back(), 0));
        selectRow (search_index.back());
        /*QTimer::singleShot (1, this, [this]()
        {
            scrollTo (model()->index (search_index.back()));
        });*/
    }
}

void MessageLog::append (const QString& text, const QString& icon, const QDateTime& dateTime, bool file)
{
    // setContentsMargins (QMargins (8, 8, 8, 8));
    const bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();

    static_cast<CachedModel*>(model())->append (text, icon, dateTime, file);

    resizeRowToContents (model()->rowCount()-1);
    //qDebug()<<"SA: Resizing row"<<model()->rowCount()-1;

    if (atBottom)
        scrollToBottom();
}

void MessageLog::clear()
{
    //STUB
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
   // const bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();

    for (int i = firstIndex; i<=lastIndex;++i)
    {
        resizeRowToContents(i);
        //qDebug()<<"Resizing row"<<i;
    }

    //if (atBottom)
        //QTimer::singleShot (1, this, &QTableView::scrollToBottom);

    //scheduleDelayedItemsLayout();
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

CachedModel::CachedModel (QObject* parent, const QString& _table): QAbstractListModel (parent), cache (50)
{
    row_count = 0;
    table = _table;

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query (db);
    query.exec ("create table IF NOT EXISTS '"+table+"' (id INTEGER PRIMARY KEY AUTOINCREMENT, text TEXT, pixmap TEXT, datetime TEXT, file INTEGER)");

    query.prepare ("SELECT MAX(id) FROM '"+table+"'");
    query.exec();
    while (query.next())
    {
        row_count =  query.value(0).toInt();
        qDebug()<<"Row count read:"<<row_count;
    }

    if (row_count>0)
    {
        beginInsertRows (QModelIndex(), 0, row_count-1);
        endInsertRows();
    }
}

QVariant CachedModel::data (const QModelIndex& _index, int role) const
{    
    if (!_index.isValid())
        return QVariant();
    if (_index.row() >= rowCount()|| _index.row() < 0)
        return QVariant();

    //qDebug()<<"Cached range before:"<< cache.firstIndex()<<"-"<<cache.lastIndex();
    int old_firstIndex = cache.firstIndex();
    int old_lastIndex = cache.lastIndex();
    const int lookAhead (10);
    const int halfLookAhead (lookAhead/2);

    int row = _index.row();
    if (row > cache.lastIndex()) {
        if (row - cache.lastIndex() > lookAhead)
            cacheRows (row-halfLookAhead, qMin (rowCount()-1, row+halfLookAhead));
        else
        {
            while (row > cache.lastIndex())
                cache.append (fetchRow (cache.lastIndex()+1));
        }
    } else if (row < cache.firstIndex()) {
        if (cache.firstIndex() - row > lookAhead)
            cacheRows (qMax (0, row-halfLookAhead), row+halfLookAhead);
        else
        {
            while (row < cache.firstIndex())
                cache.prepend (fetchRow (cache.firstIndex()-1));
        }
    }
    //qDebug()<<"Cached range after:"<< cache.firstIndex()<<"-"<<cache.lastIndex();

    if ((cache.firstIndex()<old_firstIndex)||(cache.lastIndex()>old_lastIndex))
        emit window_changed (cache.firstIndex(), cache.lastIndex());

    return cache.at(row).value(role);
}

Qt::ItemFlags CachedModel::flags (const QModelIndex& index) const
{
    if (!data(index, Qt::UserRole+1).toBool())
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    else
    {
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
}

int CachedModel::rowCount (const QModelIndex&) const
{
    return row_count;
}

void CachedModel::cacheRows (int from, int to) const
{
    for (int i = from; i <= to; ++i)
        cache.insert (i, fetchRow (i));
}

QMap<int, QVariant> CachedModel::fetchRow (int position) const
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query (db);
    query.prepare ("select * from '"+table+"' where id is :pos");
    query.bindValue(":pos", position+1);
    query.exec();

    QMap<int, QVariant> result;
    while (query.next())
    {
        result.insert (Qt::DisplayRole, query.value(1));
        result.insert (Qt::DecorationRole, QIcon (query.value(2).toString()));
        result.insert (Qt::UserRole, query.value(3).toDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        if (query.value(4).toBool())
            result.insert (Qt::UserRole+1, query.value(1));
    }
    return result;
}

QList<int> CachedModel::search (const QString& text)
{
    if (text.length()<2)
        return QList<int>();

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query (db);
    query.prepare ("select * from '"+table+"' where text like :text");
    query.bindValue (":text", "%"+text+"%");
    query.exec();

    QList<int> result;
    while (query.next())
    {
        if (!query.value(4).toBool())
        {
            result.push_back (query.value(0).toInt()-1);
        }
    }
    return result;
}

bool CachedModel::append (const QString& text, const QString& icon, const QDateTime& dateTime, bool file)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query (db);
    query.prepare ("INSERT INTO '"+table+"' (text, pixmap, datetime, file) "
                  "VALUES (:text, :pixmap, :datetime, :file)");
    query.bindValue (":text", text);
    query.bindValue (":pixmap", icon);
    query.bindValue (":datetime", dateTime);
    query.bindValue (":file", file);

    ++row_count;
    beginInsertRows (QModelIndex(), rowCount()-1, rowCount()-1);
    bool result = query.exec();
    endInsertRows();

    return result;
}