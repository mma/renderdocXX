
#include "NodeLink.h"
#include <QPainter>

NodeLink::NodeLink(MySlot *s1, MySlot *s2) : startSlot(s1), endSlot(s2)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
}

NodeLink::NodeLink(QPointF f1, QPointF f2) : mf1(f1), mf2(f2)
{
}

NodeLink::~NodeLink()
{

}

void NodeLink::updateShape()
{
    if(!startSlot || !endSlot || !startSlot->isVisible() || !endSlot->isVisible())
        return;

    QPointF beginPos = startSlot->GetAnchor();
    QPointF endPos = endSlot->GetAnchor();
    QPainterPath path;
    path.moveTo(beginPos);
    if (beginPos.y() > endPos.y()) {
        std::swap(beginPos, endPos);
    }
    //使用三阶贝塞尔曲线连接两个点
    qreal dy = endPos.y() - beginPos.y();
    if (dy > 60)
    {
        dy = 60;
    }
    QPointF offset(0.0f, dy);
    path.cubicTo(beginPos + offset, endPos - offset, endPos);
    setPath(path);//设置到当前 QGraphicsPathItem；
    update();
}

void NodeLink::updateShape1()
{
    //使用 mf1, mf2 而不是 startSlot, endSlot；
    QPointF beginPos = mf1;
    QPointF endPos = mf2;
    QPainterPath path;
    path.moveTo(beginPos);
    if (beginPos.y() > endPos.y()) {
        std::swap(beginPos, endPos);
    }
    qreal dy = endPos.y() - beginPos.y();
    if (dy > 60)
    {
        dy = 60;
    }
    QPointF offset(0.0f, dy);
    path.cubicTo(beginPos + offset, endPos - offset, endPos);
    setPath(path);
    update();
}

void NodeLink::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing);
    QGraphicsPathItem::paint(painter, option, widget);
}

QPainterPath NodeLink::shape() const
{
    // 控制点击区域范围
    QPainterPathStroker s;
    s.setWidth(10);
    return s.createStroke(QGraphicsPathItem::shape());
}
