#pragma once

#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>

class BaseGraphicsWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    BaseGraphicsWidget(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = Qt::WindowFlags());
    ~BaseGraphicsWidget();
    virtual void Invalidate();
    virtual void UpdateLayout();
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value)override;
    virtual QRectF boundingRect()const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    void Invalidate(QGraphicsLayout *l);
};