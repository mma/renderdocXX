
#include "ComposeWidget.h"
#include <QDebug>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include "DependencyScene.h"
#include <QCoreApplication>
#include <QTimer>

ComposeWidget::ComposeWidget(int itemCount, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : BaseGraphicsWidget(parent, wFlags)
{
    mVLayout = new QGraphicsLinearLayout(Qt::Orientation::Vertical);//外层垂直布局；
    mHFlagLayout = new QGraphicsLinearLayout(Qt::Orientation::Horizontal);//顶部中间的“按钮/标签”行；
    mHContentsLayout = new QGraphicsLinearLayout(Qt::Orientation::Horizontal);//中间的内容行，含多个图文项。
    
    //把两个水平布局加到纵向容器中，垂直堆叠。
    mVLayout->addItem(mHFlagLayout);
    mVLayout->setSpacing(0);
    mVLayout->addItem(mHContentsLayout);

    //加入图文项 两侧加 addStretch() 让它们居中。
    mHContentsLayout->addStretch();
    for(int i = 0; i < itemCount; ++i)
    {
      auto tap = std::make_shared<PassItem>();
      mLayoutItems.push_back(tap);    // 保存到成员列表
      mHContentsLayout->addItem(tap.get());
    }

    mHContentsLayout->addStretch();// 补充末尾Stretch

    //将一个 FlagNameItem 居中放在顶部，用于点击触发展开/收起。
    mFlag = std::make_shared<FlagNameItem>();
    mHFlagLayout->addStretch();
    mHFlagLayout->addItem(mFlag.get());
    mHFlagLayout->addStretch();

    //将 mVLayout 设置为当前控件的布局。
    setLayout(mVLayout);

    mAnimation = std::make_shared<QPropertyAnimation>(this, "rs");//创建一个属性动画，作用在 this 的 "rs" 属性上，用于控制尺寸变化。
    connect(mFlag.get(), &FlagNameItem::Clicked, this, &ComposeWidget::StateChange);//点击顶部 mFlag 调用 StateChange()，触发展开/收起动画。
    connect(mAnimation.get(), &QPropertyAnimation::valueChanged, this, &ComposeWidget::SizeChanged);//动画值变化时触发 SizeChanged()，实时设置控件尺寸。

    mMenu = std::make_shared<DependencyMenu>();
}

ComposeWidget::~ComposeWidget()
{

}

void ComposeWidget::SizeChanged(const QVariant &value)
{
}

void ComposeWidget::setRS(QSizeF f)
{
    rs = f;
    resize(f);
    if (mZoomIn && property("mysize").toSizeF() == f) {
      for(auto &item : mLayoutItems)
      {
        item->setVisible(!item->isVisible());
      }
    }
}

void ComposeWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter->setBrush(QColor(211, 211, 211));
    painter->drawRoundedRect(boundingRect(), 5, 5);
}

QVariant ComposeWidget::itemChange(GraphicsItemChange change, const QVariant& value)
{
    return QGraphicsWidget::itemChange(change, value);
}

void ComposeWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    mMenu->exec(QCursor::pos());
}

void ComposeWidget::StateChange()
{
    for(auto &item : mLayoutItems)
    {
        item->setVisible(!item->isVisible());
    }

    Invalidate(); // 触发布局刷新

    for(auto &item : mLayoutItems)
    {
      item->UpdateLayout();
    }

    QSizeF targetSize = effectiveSizeHint(Qt::SizeHint::PreferredSize);
    mZoomIn = false;
    setProperty("mysize", targetSize);

    if(mLayoutItems.front()->isVisible())
    {
      mZoomIn = true;

      for(auto &item : mLayoutItems)
      {
        item->setVisible(!item->isVisible());
      }
    }

    mAnimation->setStartValue(size());
    mAnimation->setEndValue(targetSize);
    mAnimation->setDuration(333);

    mAnimation->start();

}
