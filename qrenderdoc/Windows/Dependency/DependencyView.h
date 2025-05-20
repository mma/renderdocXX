#pragma once

#include <QGraphicsView>
#include "DependencyScene.h"
#include <memory>

class DependencyView : public QGraphicsView
{
    Q_OBJECT
public:
    DependencyView(QWidget *parent = nullptr);
  ~DependencyView();

protected:
    virtual void wheelEvent(QWheelEvent* event);
    void showEvent(QShowEvent *event) override;

private:
    std::shared_ptr<DependencyScene> mScene;

signals:
    void ZoomScale(QString s);
};
