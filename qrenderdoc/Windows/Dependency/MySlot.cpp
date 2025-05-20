
#include "MySlot.h"
#include <QPainter>
#include <QDebug>

MySlot::MySlot(QGraphicsItem *parent) : QGraphicsRectItem(parent)
{
   // setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}

QPointF MySlot::GetAnchor()
{
    //使用 mapToScene() 将本地坐标系中的中心点转换为场景坐标系中的绝对位置；
    QPointF endPos = mapToScene(boundingRect().center());
    return endPos;
}

QRectF MySlot::boundingRect() const
{
    //控制锚点大小
    return QRectF(0, 0, 2, 2);
}

void MySlot::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
//    painter->setRenderHint(QPainter::Antialiasing);
//    painter->drawRoundedRect(boundingRect(), 5, 5);
}
