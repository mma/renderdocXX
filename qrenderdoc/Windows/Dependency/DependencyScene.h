#pragma once

#include <QGraphicsScene>
#include "MySlot.h"
#include "NodeLink.h"

class DependencyScene : public QGraphicsScene
{
    Q_OBJECT
public:
    DependencyScene(QObject *parent = nullptr);
    void AddLink(MySlot *s1, MySlot *s2);
    QVector<NodeLink *> links;
    void Update();
    void SetCanUpdate(bool f) { mCanUpdate = f; }

protected:
    bool mCanUpdate = true;
};