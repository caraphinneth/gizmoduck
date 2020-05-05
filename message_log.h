#pragma once

#include <QListView>
#include <QStyledItemDelegate>
#include <QLabel>
#include <QTextDocument>
#include <QTextCursor>
#include <QStandardItemModel>
#include <QContiguousCache>
#include <QSqlQuery>

class MessageLog : public QListView
{
    Q_OBJECT

public:
    explicit MessageLog (QWidget *parent = nullptr);

public slots:

    void append (const QString &text, const QPixmap &pixmap, const QDateTime &dateTime, bool file = false);
    void clear();

private:
    void resizeEvent (QResizeEvent *event);
};

class Message : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit Message (QObject *parent = nullptr);

    QSize iconSize;
    QMargins margins;
    int horizontalSpacing;
    int verticalSpacing;

    QTextDocument content;

    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    inline QRect timestampBox (const QStyleOptionViewItem &option, const QModelIndex &index) const;
    inline qreal timestampFontPointSize (const QFont &f) const;
    inline QTextDocument* messageBox (const QStyleOptionViewItem &option, const QModelIndex &index, bool fastmode=false) const;
};

class MessageEditor: public QLabel
{
    Q_OBJECT

public:
    explicit MessageEditor (QWidget *parent = nullptr);
    // QSize sizeHint() const override;

protected:
    //void paintEvent(QPaintEvent *event) override;
};

class CachedModel : public QStandardItemModel
{
    Q_OBJECT

public:
    CachedModel (QObject *parent = nullptr);
    QVariant data (const QModelIndex &, int) const override;

private:
    void cacheRows (int, int) const;
    QMap<int, QVariant> fetchRow (int position) const;
    mutable QContiguousCache<QMap<int, QVariant>> cache;
    QSqlDatabase db;
};