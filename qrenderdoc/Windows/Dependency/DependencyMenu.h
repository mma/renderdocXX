#pragma once

#include <QMenu>
#include <QWidgetAction>
#include <QHBoxLayout>
#include <QPixmap>
#include <QLabel>
#include <memory>

class MenuWidget;
class DependencyMenu : public QMenu
{
public:
    DependencyMenu(QWidget *parent = nullptr);
    DependencyMenu(const QString &title, QWidget *parent = nullptr);
    ~DependencyMenu();

private:
    QVector<QAction *> mActions;
    std::shared_ptr<QMenu> mSubMenu;
    QVector<MenuWidget *> mMenuWidgets;
};

class MenuWidget : public QWidget
{
    Q_OBJECT
public:
    MenuWidget(QWidget *parent = 0);
    ~MenuWidget();

protected:
    void paintEvent(QPaintEvent *event) override;
  void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    std::shared_ptr<QHBoxLayout> mLayout;
    std::shared_ptr<QLabel> mImg;
    std::shared_ptr<QLabel> mInfo;
    bool mIsActived = false;
};
