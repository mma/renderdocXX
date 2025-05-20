
#include "InfosItem.h"

InfosItem::InfosItem(QGraphicsItem *parent, Qt::WindowFlags wFlags) : BaseGraphicsWidget(parent, wFlags)
{
    mLayout = new QGraphicsLinearLayout(Qt::Orientation::Vertical);
    for (int i = 0; i < 5; ++i) {
        GraphicsPixmapTitleItem *ti = new GraphicsPixmapTitleItem();
        mLayout->addItem(ti);
        mItems.push_back(ti);
    }
    setLayout(mLayout);
}

InfosItem::~InfosItem()
{
    for (GraphicsPixmapTitleItem *ti : mItems) {
        delete ti;
    }
}
