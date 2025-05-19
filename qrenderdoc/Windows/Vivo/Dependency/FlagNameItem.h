#pragma once

#include "BaseLayoutItem.h"

class FlagNameItem : public BaseLayoutItem
{
    Q_OBJECT
public:
    FlagNameItem(QGraphicsLayoutItem *parent = nullptr, bool isLayout = false);
    ~FlagNameItem();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    virtual void setGeometry(const QRectF &rect)override;
private:
    QString mLabel;

signals:
    void Clicked();
};