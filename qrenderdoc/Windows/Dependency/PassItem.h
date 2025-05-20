#pragma once

#include <memory>
#include <QGraphicsLinearLayout>
#include "InfosItem.h"
#include "BaseGraphicsWidget.h"
#include "GraphicsPixmapItem.h"
#include "GraphicsPixmapTitleItem.h"
#include "dependency_info.h"

using namespace vivo;  

struct InputInfo
{
  uint32_t drawEid;
  uint32_t slot;
  uint32_t resourceId;
  QString subres;
  QString usage;
};

class PassItem : public BaseGraphicsWidget
{
    Q_OBJECT
public:
    PassItem(QGraphicsItem *parent = nullptr);
  ~PassItem();
    MySlot *GetInAnchor() { return mTItem->GetAnchor(); }
    MySlot *GetOutAnchor() { return mItem->GetSlot(); }

    void SetTitleText(const QString &text);
    void SetTitle(const QString &txt); 
    void SetSize(int w, int h);

    void SetInputInfoList(const rdcarray<DepInputInfo> &inputs);
    void SetOutputInfoList(const rdcarray<DepOutputInfo> &outputs); 

    rdcarray<DepOutputInfo> m_OutputInfos;

    QGraphicsLinearLayout *mLayout;
    std::shared_ptr<GraphicsPixmapTitleItem> mTItem;
    std::shared_ptr<GraphicsPixmapItem> mItem;
    std::shared_ptr<InfosItem> mInfos;
    std::vector<std::shared_ptr<GraphicsPixmapItem>> mOutputs;

    DepPassInfo mPassInfo;

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value)override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
private:
};