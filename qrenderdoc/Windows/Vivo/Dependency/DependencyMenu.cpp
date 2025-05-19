
#include "DependencyMenu.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QDebug>

static const QString qss = QStringLiteral(R"(
QMenu {
    background-color: rgb(255,255,255);
    padding: 5px;
    margin: 5px;
    border: 1px solid gray;
    border-radius: 10px;
}
QMenu::item {
    font-size:10pt;
    color: rgb(0,0,0);
    background-color: transparent;
    padding: 8px 25px 6px 10px;
    margin: 4px 1px;
}
QMenu::item:selected {
    background-color: lightblue;
    border-radius: 5px;
}
QMenu::icon:checked {
    background: rgb(253,253,254);
    position: absolute;
    top: 1px;
    right: 1px;
    bottom: 1px;
    left: 1px;
}
QMenu::icon:checked:selected {
    background-color: rgb(236,236,237);
}
QMenu::separator {
    height: 2px;
    background: rgb(235,235,236);
    margin-left: 5px;
    margin-right: 5px;
}
)");


DependencyMenu::DependencyMenu(QWidget *parent) : QMenu(parent)
{
    //添加子菜单
    mSubMenu = std::make_shared<QMenu>(QStringLiteral ("new"));
    QAction *ac = new QAction(QStringLiteral("close"));
    QAction *ac1 = new QAction(QStringLiteral("open"));
    QAction *ac2 = new QAction(QStringLiteral("save"));
    QAction *ac3 = new QAction(QStringLiteral("save as another view"));
    addMenu(mSubMenu.get());
    addSeparator();
    mActions.push_back(ac);
    mActions.push_back(ac1);
    mActions.push_back(ac2);
    mActions.push_back(ac3);
    addAction(ac);
    addAction(ac1);
    addAction(ac2);
    addAction(ac3);

    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    // 设置背景透明，让四个角只显示圆角，不显示其他黑的部分
    setAttribute(Qt::WA_TranslucentBackground);
    mSubMenu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    mSubMenu->setAttribute(Qt::WA_TranslucentBackground);

    //每个 MenuWidget 是一个嵌入的菜单项，用 QWidgetAction 包装后加入菜单；
    for (int i = 0; i < 5; ++i) {
        QWidgetAction *wa = new QWidgetAction(mSubMenu.get());
        MenuWidget *mw = new MenuWidget();
        mMenuWidgets.push_back(mw);
        wa->setDefaultWidget(mw);
        mSubMenu->addAction(wa);
    }

    setStyleSheet(qss);              // 给当前菜单（DependencyMenu）应用样式
    mSubMenu->setStyleSheet(qss);    // 给子菜单 mSubMenu 也应用样式
}

DependencyMenu::DependencyMenu(const QString &title, QWidget *parent) : DependencyMenu(parent)
{
    setTitle(title);
}

DependencyMenu::~DependencyMenu()
{
    for (MenuWidget *ac : mMenuWidgets) {
        delete ac;
    }
    for (QAction *ac : mActions) {
        delete ac;
    }
}

MenuWidget::MenuWidget(QWidget *parent) : QWidget(parent)
{
    //水平布局，添加了左侧图标、右侧描述
    mLayout = std::make_shared<QHBoxLayout>();
    setLayout(mLayout.get());
    mLayout->setSpacing(30);
    mImg = std::make_shared<QLabel>(QStringLiteral ("???"));

    //图标使用灰色填充的小正方形。
    QPixmap pix(20, 20);
    pix.fill(Qt::gray);
    mImg->setPixmap(pix);
    mInfo = std::make_shared<QLabel>(QStringLiteral ("info"));
    mLayout->addWidget(mImg.get());
    mLayout->addWidget(mInfo.get());
    setMouseTracking(true);//使 enterEvent 在鼠标进入时触发，而无需点击。
}

MenuWidget::~MenuWidget()
{

}

void MenuWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    if (mIsActived) {
        p.setBrush(QColor(173, 216, 230));//当 mIsActived 为真，绘制一层蓝色圆角高亮背景
        p.drawRoundedRect(rect(), 5, 5);
    }
    QWidget::paintEvent(event);
}

void MenuWidget::enterEvent(QEvent *event)
{
    mIsActived = true;//控制 mIsActived 状态切换
    update();//使用 update() 触发 paintEvent() 重绘
    QWidget::enterEvent(event);
}

void MenuWidget::leaveEvent(QEvent *event)
{
    mIsActived = false;
    update();
    QWidget::leaveEvent(event);
}
