
#include "GraphicsPixmapItem.h"
#include <QPixmap>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QImageReader>

GraphicsPixmapItem::GraphicsPixmapItem(QGraphicsLayoutItem *parent, bool isLayout) : BaseLayoutItem(parent, isLayout)
{
//    setFlag(QGraphicsItem::ItemIsSelectable);
    mItem = std::make_shared<QGraphicsPixmapItem>();
    mItem->setParentItem(this);

    //test：加载图片，后面根据需要删减调整
    QString imagePath = QStringLiteral("E:/2.png");
    QPixmap p(imagePath);
    if(p.isNull())
    {
      qDebug() << QStringLiteral("图片加载失败") << imagePath;
    }
    else
    {
      p = p.scaledToWidth(150);
      mItem->setPixmap(p);
    }

    mOut = std::make_shared<MySlot>(this);
}

GraphicsPixmapItem::~GraphicsPixmapItem()
{

}

//控件的绘制/交互区域，取决于子项 mItem 的大小；若当前控件不可见，返回空矩形，避免冗余绘制。
QRectF GraphicsPixmapItem::boundingRect() const
{
    if (isVisible()) {
    return QRectF(0, 0, mItem->boundingRect().width(), mItem->boundingRect().height());
    } else {
        return QRectF();
    }
}

void GraphicsPixmapItem::setGeometry(const QRectF &rect)
{
    BaseLayoutItem::setGeometry(rect);//更新位置/大小
    mOut->setPos(rect.width() / 2, rect.height());// 设置输出锚点位置（底部中间）
}

QVariant GraphicsPixmapItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    return BaseLayoutItem::itemChange(change, value);
}
