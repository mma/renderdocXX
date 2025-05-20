#pragma once

#include "MySlot.h"
#include <QGraphicsPathItem>

class NodeLink : public QGraphicsPathItem
{
public:
    NodeLink(MySlot *s1, MySlot *s2);
    NodeLink(QPointF f1, QPointF f2);
    ~NodeLink();
    void updateShape();
    void updateShape1();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QPainterPath shape() const override;

private:
    MySlot *startSlot;
    MySlot *endSlot;
    QPointF mf1;
    QPointF mf2;
};