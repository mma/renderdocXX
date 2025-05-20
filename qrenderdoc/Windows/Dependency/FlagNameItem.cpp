
#include "FlagNameItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <QString>

namespace  {
constexpr qreal MARGIN = 12.0;
constexpr qreal INNER_SPACING = 5.0;
constexpr qreal INNER_MARGIN = 5.0;
constexpr qreal BORDER_RADIUS = 5.0;
}

FlagNameItem::FlagNameItem(QGraphicsLayoutItem *parent, bool isLayout) : BaseLayoutItem(parent, isLayout)
{
    mItemSize = QSize(250, 80);
    setAcceptHoverEvents(true);
    mLabel = QStringLiteral("this is just a DP demo !!!");
}

FlagNameItem::~FlagNameItem()
{

}

void FlagNameItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
    QGraphicsObject::hoverMoveEvent(e);
    update();
}

void FlagNameItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
    QGraphicsObject::hoverLeaveEvent(e);
    update();
}

void FlagNameItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    QGraphicsObject::mousePressEvent(e);
    Clicked();
}

void FlagNameItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    QPen pen = painter->pen();
    QRectF rect = boundingRect();
    QRectF borderRect = rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

    painter->setPen(Qt::NoPen);
    if (option->state.testFlag(QStyle::State_MouseOver)) {//鼠标悬浮时，设置背景设为蓝色 高亮
        painter->setBrush(QColor(58,139,243));
        painter->drawRoundedRect(rect, 8, 8);
    }
    // 绘制主题区域
    painter->setBrush(QColor(222,196,136));
    painter->drawRoundedRect(borderRect, BORDER_RADIUS, BORDER_RADIUS);

    //内部内容再次留白，变成 contentsRect；左侧是一个方形 flag 区域（高=宽），填充为橙色。
    QRectF contentsRect = borderRect.adjusted(INNER_MARGIN, INNER_MARGIN, -INNER_MARGIN, -INNER_MARGIN);
    QRectF flag(contentsRect.x(), contentsRect.y(), contentsRect.height(), contentsRect.height());
    painter->fillRect(flag, QColor(225,173,111));

    painter->setPen(pen);
    painter->save();

    QFont font = painter->font();
    font.setBold(true);
    font.setPointSize(flag.height() / 2);
    painter->setFont(font);
    painter->drawText(flag, Qt::AlignCenter, QStringLiteral ("ID"));
    painter->restore();

    //文字区域位于 flag 右边，带一定内间距;使用 elidedText 自动截断超长文字并加“…”
    QRectF labelRect = contentsRect.adjusted(flag.width() + INNER_SPACING, 0, 0, 0);
    QString label = painter->fontMetrics().elidedText(mLabel, Qt::ElideMiddle, labelRect.width());
    painter->drawText(labelRect, Qt::AlignCenter, label);
}

//提供控件的可点击/选中/鼠标交互区域；默认使用矩形包围框。
QPainterPath FlagNameItem::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

//设置布局分配的区域；设置布局分配的区域；更新 item 的实际位置和大小。
void FlagNameItem::setGeometry(const QRectF &rect)
{
    prepareGeometryChange();
    QGraphicsLayoutItem::setGeometry(rect);
    setPos(rect.topLeft());
}
