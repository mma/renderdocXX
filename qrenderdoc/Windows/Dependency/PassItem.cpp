#include "PassItem.h"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include "DependencyScene.h"

PassItem::PassItem(QGraphicsItem *parent) : BaseGraphicsWidget(parent)
{
  setFlag(ItemIsMovable, false);
  setFlag(ItemIsSelectable, false);

  // 外部垂直布局
  mLayout = new QGraphicsLinearLayout(Qt::Vertical);
  mLayout->setSpacing(5);
  mLayout->setContentsMargins(5, 5, 5, 5);

  mTItem = std::make_shared<GraphicsPixmapTitleItem>();
  mLayout->addItem(mTItem.get());

  // ---------- 输入 Binding 横排 ----------
  mInputLayout = std::make_shared<QGraphicsLinearLayout>(Qt::Horizontal);
  mInputWidget = std::make_shared<QGraphicsWidget>();
  mInputWidget->setLayout(mInputLayout.get());
  mLayout->addItem(mInputWidget.get());

  // ---------- 输出图像横排 ----------
  mOutputLayout = std::make_shared<QGraphicsLinearLayout>(Qt::Horizontal);
  mOutputWidget = std::make_shared<QGraphicsWidget>();
  mOutputWidget->setLayout(mOutputLayout.get());
  mLayout->addItem(mOutputWidget.get());
  setLayout(mLayout);

  adjustSize();
}

PassItem::~PassItem()
{
  setLayout(nullptr);
}

QRectF PassItem::boundingRect() const
{
  // 利用 BaseGraphicsWidget 提供的 sizeHint() 来动态获取大小；
  return QRectF(0, 0, this->sizeHint(Qt::PreferredSize).width(),
                this->sizeHint(Qt::PreferredSize).height());
}

void PassItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // 启用抗锯齿；在鼠标悬浮时绘制浅蓝圆角背景；
  painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                          QPainter::SmoothPixmapTransform);
  QRectF rect = boundingRect().adjusted(1, 1, -1, -1);    // 留出边缘

  // 背景色：默认淡灰，悬浮时变淡蓝
  QColor bgColor = option->state.testFlag(QStyle::State_MouseOver)
                       ? QColor(200, 220, 250)     // hover 时高亮
                       : QColor(245, 245, 245);    // 默认背景色

  // 边框色
  QColor borderColor(180, 180, 180);

  // 背景填充
  painter->setPen(Qt::NoPen);
  painter->setBrush(bgColor);
  painter->drawRoundedRect(rect, 6, 6);

  // 边框绘制
  painter->setPen(QPen(borderColor, 1));
  painter->setBrush(Qt::NoBrush);
  painter->drawRoundedRect(rect, 6, 6);
}

QVariant PassItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
  if(scene() != nullptr)
  {
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
  while(mInputLayout->count() > 0)
  {
    QGraphicsLayoutItem *item = mInputLayout->itemAt(0);
    mInputLayout->removeAt(0);
    delete item;
  }

  for(const auto &input : inputs)
  {
    QString imageStr = QString::fromStdString(ToStr(input.m_ImageInfo.m_ImageID).c_str());
    QString usageStr = QString::fromUtf8(input.m_Usage.c_str());
    QString subresStr = QStringList::fromVector(QVector<QString>::fromStdVector(std::vector<QString>(
                                                    input.m_ImageInfo.m_Subresources.begin(),
                                                    input.m_ImageInfo.m_Subresources.end())))
                            .join(QStringLiteral(","));
    QString shortLabel =
        QStringLiteral("Slot %1\nDraw %2\nImg %3").arg(input.m_BindSlot).arg(input.m_DrawEid).arg(imageStr);

    QString fullTip = QStringLiteral("Slot: %1\nDrawEID: %2\nUsage: %3\nImgID: %4\nSubres: %5")
                          .arg(input.m_BindSlot)
                          .arg(input.m_DrawEid)
                          .arg(usageStr)
                          .arg(imageStr)
                          .arg(subresStr);

    auto *item = new GraphicsPixmapTitleItem();
    item->SetTitle(shortLabel, fullTip);
    mInputLayout->addItem(item);
  }
  UpdateLayout();
}

void PassItem::SetOutputInfoList(const rdcarray<DepOutputInfo> &outputs)
{
  m_OutputInfos = outputs;

  while(mOutputLayout->count() > 0)
  {
    QGraphicsLayoutItem *item = mOutputLayout->itemAt(0);
    mOutputLayout->removeAt(0);
    delete item;
  }

  for(const auto &out : outputs)
  {
    QString imageStr = QString::fromStdString(ToStr(out.m_ImageInfo.m_ImageID).c_str());
    QString formatStr = QString::fromUtf8(out.m_Format.Name().c_str());

    QString label =
        QStringLiteral("ImgID %1\n%2\n%3x%4").arg(imageStr).arg(formatStr).arg(out.m_Width).arg(out.m_Height);

    QString subresStr = QStringList::fromVector(QVector<QString>::fromStdVector(std::vector<QString>(
                                                    out.m_ImageInfo.m_Subresources.begin(),
                                                    out.m_ImageInfo.m_Subresources.end())))
                            .join(QStringLiteral(","));

    QString tooltip = QStringLiteral("ImgID: %1\nFormat: %2\nSize: %3x%4\nSubres: %5")
                          .arg(imageStr)
                          .arg(formatStr)
                          .arg(out.m_Width)
                          .arg(out.m_Height)
                          .arg(subresStr);

    auto *item = new GraphicsPixmapTitleItem();
    item->SetTitle(label, tooltip);
    mOutputLayout->addItem(item);
  }
  UpdateLayout();
}