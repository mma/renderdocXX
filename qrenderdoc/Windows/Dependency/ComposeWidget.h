#pragma once

#include "BaseGraphicsWidget.h"
#include <QGraphicsLinearLayout>
#include <QPropertyAnimation>
#include "PassItem.h"
#include "FlagNameItem.h"
#include <QAction>
#include "DependencyMenu.h"
#include <memory>

class ComposeWidget : public BaseGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(QSizeF rs READ getRS WRITE setRS)
public:
    ComposeWidget(int itemCount, QGraphicsItem *parent = nullptr,
                  Qt::WindowFlags wFlags = Qt::WindowFlags());
    ~ComposeWidget();
    QSizeF getRS() { return rs; };
    void setRS(QSizeF f);
    QVector<PassItem *> mItems;
    std::vector<std::shared_ptr<PassItem>> mLayoutItems;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value)override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    QGraphicsLinearLayout *mVLayout;
    QGraphicsLinearLayout *mHFlagLayout;
    QGraphicsLinearLayout *mHContentsLayout;

    std::shared_ptr<FlagNameItem> mFlag;

    std::shared_ptr<QPropertyAnimation> mAnimation;

    std::shared_ptr<DependencyMenu> mMenu;
    bool mZoomIn = false;
    QSizeF rs;

private slots:
    void StateChange();
    void SizeChanged(const QVariant &value);
};
