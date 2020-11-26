#include <QAbstractItemDelegate>
#include <QPushButton>
#include <QStylePainter>
#include <QStyleOptionTab>
#include <QWheelEvent>

#include "side_tabs.h"

#include <QDebug>

// Side tabs, polishing still needed.
SideTabs::SideTabs (QWidget* parent, int w, int h): QTabBar (parent)
{
    setMinimumSize (h, w);
    setStyleSheet (QString ("QTabBar::tab { height: %1px; width: %2px; }").arg(h).arg(w)); // QTabBar::scroller {height:%1pix;width:%2pix;}
    //setStyleSheet ("QTabBar::close-button { padding-left: 64px; }");
    loading_icon = new QMovie (":/icons/loading");
    loading_icon->start();

    // Instead, you can just set a low fps icon. Not to mention this forced repaint is crude.
    connect (loading_icon, &QMovie::frameChanged, [this]()
    {
        for (int i=0; i<count(); ++i)
        {
            if (tabIcon(i).isNull())
            {
                repaint();
                break;
            }
        }
    });

}

void SideTabs::paintEvent (QPaintEvent*)
{
    QStylePainter painter (this);
    QStyleOptionTab option;
    QPixmap pic;

    for (int i=0; i<count(); ++i)
    {
        initStyleOption (&option, i);
        QRect rect = tabRect (i);
        option.icon = QIcon();
        option.text = QString();

        QRect text_rect = rect.adjusted( 26, 3, -5, -5);
        if (tabIcon(i).isNull())
            pic = loading_icon->currentPixmap();
        else
            pic = tabIcon(i).pixmap (tabIcon(i).actualSize (QSize (16, 16)));
        rect.setSize (QSize (16,16));
        rect.adjust (5,10,5,10);
        painter.drawControl (QStyle::CE_TabBarTabShape, option);
        painter.drawPixmap (rect, pic);

        QFontMetrics fontMetric (painter.font());
        const QString elidedText = fontMetric.elidedText (tabText (i), Qt::ElideRight, text_rect.width()*2);
        QTextOption t_option;
        t_option.setWrapMode (QTextOption::WrapAtWordBoundaryOrAnywhere);
        QFont condensed ("Roboto Condensed Medium", 10);
        //condensed.setHintingPreference (QFont::PreferDefaultHinting);
        painter.setFont (condensed);
        painter.drawText (text_rect, elidedText, t_option);
    }
    painter.end();
}

QSize SideTabs::tabSizeHint (int /*index*/)
{
    return minimumSize();
}


void SideTabs::wheelEvent (QWheelEvent* event)
{
    // Pass to TabWidget.
    event->ignore();
}