#pragma once

#include "BaseLayoutItem.h"
#include "MySlot.h"
#include <memory>

class GraphicsPixmapTitleItem : public BaseLayoutItem
{
    Q_OBJECT
public:
    GraphicsPixmapTitleItem(QGraphicsLayoutItem *parent = nullptr, bool is = false);
    ~GraphicsPixmapTitleItem();
    virtual QRectF boundingRect()const override;
    virtual void setGeometry(const QRectF &rect)override;
    MySlot *GetAnchor() { return mIn.get(); };
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
private:
    std::shared_ptr<QGraphicsSimpleTextItem> mItem;
    std::shared_ptr<MySlot> mIn;
};