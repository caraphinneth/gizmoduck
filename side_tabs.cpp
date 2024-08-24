#include <QPainter>
#include <QStandardItemModel>

#include "side_tabs.h"

SideTabs::SideTabs(QWidget* parent, int w, int h): QListView(parent)
{
    setFixedWidth (w);

    //loading_icon = new QMovie (":/icons/loading");
    //loading_icon->start();

    auto* tab = new TabHeader(this, w, h);

    QPalette p(palette());

    p.setBrush(QPalette::Base, QColor("#EFEFEF")); // background
    p.setBrush(QPalette::Light, QColor("#c1c1c1")); // inactive tab
    p.setBrush(QPalette::Mid, QColor("#616b71")); // borders
    p.setBrush(QPalette::Highlight, QColor("#EFEFEF")); //active tab
    p.setBrush(QPalette::Text, QColor("#616b71"));

    setPalette (p);
    //setFont (QFont ("Noto Sans Display ExtraCondensed ExtraBold", 10));
    setFont (QFont ("Roboto Condensed Medium", 10));

    setWordWrap(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setUniformItemSizes(true);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDropIndicatorShown(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setDefaultDropAction(Qt::MoveAction);
    setEditTriggers(NoEditTriggers);
    setModel(new QStandardItemModel(this));

    setItemDelegate(tab);

    /* FIXME: only update the animating tab
    connect (loading_icon, &QMovie::frameChanged, [this]()
    {
        dataChanged (model()->index (0,1), model()->index (model()->rowCount()-1, 0), QVector<int>(Qt::DecorationRole));
    });
    */

}

QWidget* SideTabs::widget (int index)
{
    return reinterpret_cast<QWidget*>(model()->index(index, 0).data (Qt::UserRole).value<quintptr>());
}

// Don't look at me like that. QLayout and thus QStackedWidget, and thus QTabWidget do iteration to retrieve indexOf(). So do we.
int SideTabs::indexOf (QWidget* widget)
{
    for (int i = 0; i < model()->rowCount(); ++i)
    {
        QModelIndex index = model()->index (i, 0);
        QWidget* w  = reinterpret_cast<QWidget*>(index.data (Qt::UserRole).value<quintptr>());
        if (w == widget)
        {
            return i;
        }
    }
    return -1;
}

TabHeader::TabHeader (QObject* parent, int w, int h, QIcon default_icon) : QStyledItemDelegate (parent)
{
    iconSize = QSize (16, 16);
    margins = QMargins (0, 0, 0, 0);
    horizontalSpacing = 0;
    verticalSpacing = 0;
    width = w;
    height = h;
    loading_icon = default_icon;
}

void TabHeader::paint (QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt (option);
    initStyleOption (&opt, index);

    const QPalette& palette (opt.palette);
    const QRect& rect (opt.rect);
    const QRect& contentRect (rect.adjusted (margins.left(), margins.top(), -margins.right(), -margins.bottom()));
    const bool lastIndex = (index.model()->rowCount() - 1) == index.row();
    const bool hasIcon = !opt.icon.isNull();
    const int bottomEdge = rect.bottom();

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
    const QRect icon_rect = contentRect.adjusted (5, 10, 5, 10);
    if (hasIcon)
        painter->drawPixmap (icon_rect.left(), icon_rect.top(), opt.icon.pixmap(iconSize));
    else
    {
        //painter->drawPixmap (icon_rect.left(), icon_rect.top(), loading_icon.pixmap(iconSize));
    }


    // Draw message text
    QRect text_rect = contentRect.adjusted( 26, 3, -5, -5);

    painter->setPen (palette.windowText().color());
    painter->setPen (palette.text().color());

    QFontMetrics fontMetric (opt.font);
    int width = text_rect.width() * 2;
    QRegularExpression ex ("\\p{Hiragana}+|\\p{Katakana}+|\\p{Han}+", QRegularExpression::UseUnicodePropertiesOption);
    if (ex.match(opt.text).hasMatch())
        width = text_rect.width();

    const QString elidedText = fontMetric.elidedText (opt.text, Qt::ElideRight, width);
    QTextOption t_option;
    t_option.setWrapMode (QTextOption::WrapAtWordBoundaryOrAnywhere);
    //condensed.setHintingPreference (QFont::PreferDefaultHinting);

    painter->drawText (text_rect, elidedText, t_option);
    painter->restore();
}

QSize TabHeader::sizeHint (const QStyleOptionViewItem& /*option*/,  const QModelIndex& /*index*/) const
{
    return QSize (width, height);
}

void SideTabs::wheelEvent (QWheelEvent* event)
{
    emit temp_pass_wheel_event (event);
}
