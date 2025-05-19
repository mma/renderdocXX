
#include "GraphicsPixmapTitleItem.h"
#include <QDebug>

GraphicsPixmapTitleItem::GraphicsPixmapTitleItem(QGraphicsLayoutItem *parent, bool is) : BaseLayoutItem(parent, is)
{
    mItem = std::make_shared<QGraphicsSimpleTextItem>();//创建一个简单的文本项 mItem；
    mItem->setParentItem(this);
    mItem->setText(QStringLiteral("This is a VK demo!!!"));
    mIn = std::make_shared<MySlot>(this);//mIn 是一个输入点，用于连接其他图形项；
}

GraphicsPixmapTitleItem::~GraphicsPixmapTitleItem()
{

}

//控件边界由文字项的大小决定；如果不可见，则返回空区域，节省绘制资源。
QRectF GraphicsPixmapTitleItem::boundingRect() const
{
//    return BaseLayoutItem::boundingRect();
    if (isVisible()) {
    return QRectF(0, 0, mItem->boundingRect().width(), mItem->boundingRect().height());
    } else {
        return QRectF();
    }
}

void GraphicsPixmapTitleItem::setGeometry(const QRectF &rect)
{
    //调用基类更新图形项位置、大小；
    BaseLayoutItem::setGeometry(rect);
    //将 mIn 的锚点居中放置在项顶部中间;横向居中：rect.width() / 2 - mIn->width() / 2;纵向微微下移一点，以免贴边。
    mIn->setPos(rect.width() / 2 - mIn->rect().width() / 2, mIn->rect().height() / 2);
}

QVariant GraphicsPixmapTitleItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    return BaseLayoutItem::itemChange(change, value);
}


