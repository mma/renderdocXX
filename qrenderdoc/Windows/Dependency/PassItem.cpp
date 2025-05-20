#include "PassItem.h"
#include "DependencyScene.h"
#include <QDebug>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include <QStyleOptionGraphicsItem>

PassItem::PassItem(QGraphicsItem *parent) : BaseGraphicsWidget(parent)
{
    setFlag(ItemIsMovable, false);
    setFlag(ItemIsSelectable, false);

    // 三个子项分别是标题、图像、附加信息；
    mLayout = new QGraphicsLinearLayout(Qt::Orientation::Vertical);
    mTItem = std::make_shared<GraphicsPixmapTitleItem>();
    mItem = std::make_shared<GraphicsPixmapItem>();
    mInfos = std::make_shared<InfosItem>();

    mLayout->setSpacing(0);
    mLayout->addItem(mTItem.get());
    mLayout->addItem(mItem.get());
    mLayout->addItem(mInfos.get());
    setLayout(mLayout);

    adjustSize();
}

PassItem::~PassItem()
{
    setLayout(nullptr);
}

QRectF PassItem::boundingRect() const
{
    //利用 BaseGraphicsWidget 提供的 sizeHint() 来动态获取大小；
    return QRectF(0, 0, this->sizeHint(Qt::PreferredSize).width(), this->sizeHint(Qt::PreferredSize).height());
}

void PassItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    //启用抗锯齿；在鼠标悬浮时绘制浅蓝圆角背景；
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(58,139,243));
    //boundingRect() 决定绘制区域。
    if (option->state.testFlag(QStyle::State_MouseOver)) {
        painter->drawRoundedRect(boundingRect(), 10, 10);
    }
}

QVariant PassItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (scene() != nullptr) {
      static_cast<DependencyScene *>(scene())->Update();
    }
    return BaseGraphicsWidget::itemChange(change, value);
}

void PassItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
}

void PassItem::SetTitleText(const QString &text)
{
  if(mTItem)
    mTItem->SetTitle(text);
}

void PassItem::SetSize(int w, int h)
{
  mItem->SetPixmapSize(w, h);
}

void PassItem::SetTitle(const QString &txt)
{
  mTItem->SetTitle(txt);
}

void PassItem::SetInputInfoList(const rdcarray<DepInputInfo> &inputs)
{
  int i = 0;
  for(const auto &input : inputs)
  {
    if(i >= int(mInfos->mItems.size()))
      break;

    QString subres = input.m_ImageInfo.m_Subresources.empty()
                         ? QString()
                         : QStringLiteral("Sub[%1]").arg(input.m_ImageInfo.m_Subresources[0]);

    QString text = QStringLiteral("Slot %1, Draw %2, Usage: %3\nImgID %4 %5")
                       .arg(input.m_BindSlot)
                       .arg(input.m_DrawEid)
                       .arg(QString::fromUtf8(input.m_Usage.c_str()))
                       .arg(ToStr(input.m_ImageInfo.m_ImageID))
                       .arg(subres);

    mInfos->mItems[i]->SetTitle(text);
    ++i;
  }

  for(; i < int(mInfos->mItems.size()); ++i)
  {
    mInfos->mItems[i]->SetTitle(QStringLiteral(""));
  }
}

void PassItem::SetOutputInfoList(const rdcarray<DepOutputInfo> &outputs)
{
  m_OutputInfos = outputs;

  if(!outputs.empty())
  {
    const auto &info = outputs[0];
    SetSize(info.m_Width, info.m_Height);
  }
}