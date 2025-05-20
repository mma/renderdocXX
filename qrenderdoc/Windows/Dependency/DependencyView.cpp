

//实现整个图形编辑器系统提供了视图层支持，它继承自 QGraphicsView，配合 DependencyScene（图形场景）构建起了完整的 Qt Graphics View 框架中的“视图-场景”模型。
#include "DependencyView.h"
#include <QWheelEvent>
#include <qmath.h>

DependencyView::DependencyView(QWidget *parent) : QGraphicsView(parent)
{
    //设置缩放锚点为鼠标当前位置，提升用户缩放体验；
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    mScene = std::make_shared<DependencyScene>(this);
    setScene(mScene.get());
    setDragMode(DragMode::RubberBandDrag);
}

DependencyView::~DependencyView()
{

}

void DependencyView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        qreal scaleFactor = qPow(2.0, event->angleDelta().y() / 240.0);
        qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
        if(0.05 < factor && factor < 10) {
            scale(scaleFactor, scaleFactor);
          ZoomScale(QString::number(factor * 100) + QStringLiteral("%"));
        }
    }
}

void DependencyView::showEvent(QShowEvent *event)
{
  qobject_cast<DependencyScene *>(scene())->Update();
}

