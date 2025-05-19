#pragma once

#include <QGraphicsLayoutItem>
#include <QGraphicsObject>

class BaseLayoutItem : public QGraphicsObject, public QGraphicsLayoutItem
{
    Q_OBJECT
public:
    BaseLayoutItem(QGraphicsLayoutItem *parent = nullptr, bool isLayout = false);
    ~BaseLayoutItem();

    virtual QRectF boundingRect()const override;
    virtual void setGeometry(const QRectF &rect)override;

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)override;
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;
    QSizeF mItemSize;
//    void hoverEnterEvent(QGraphicsSceneHoverEvent *e)override;
//    void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;
//    void mousePressEvent(QGraphicsSceneMouseEvent *event)override;
//    void hoverMoveEvent(QGraphicsSceneHoverEvent* event)override;
//    void mouseMoveEvent(QGraphicsSceneMouseEvent* event)override;
//    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)override;
};