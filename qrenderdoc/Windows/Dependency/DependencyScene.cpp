
#include "DependencyScene.h"
#include <QPainter>
#include <QDebug>

DependencyScene::DependencyScene(QObject *parent) : QGraphicsScene(parent)
{

}

void DependencyScene::AddLink(MySlot *s1, MySlot *s2)
{
    NodeLink *l = new NodeLink(s1, s2);
    l->updateShape();
    links.push_back(l);
    addItem(l);
}

void DependencyScene::Update()
{
    if (mCanUpdate) {
        for (int i = 0; i < links.size(); i++) {
            links[i]->updateShape();
        }
    }
}

