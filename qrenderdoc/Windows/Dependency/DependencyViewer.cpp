#include "DependencyViewer.h"
#include "ui_DependencyViewer.h"
#include <QPropertyAnimation>
#include <QDebug>
#include "dependency_info.h" // 包含 vivo::DepPassInfo 等结构
using namespace vivo;

ResourceFormat MakeRGBA8Format()
{
  ResourceFormat fmt;
  fmt.type = ResourceFormatType::Regular;
  fmt.compCount = 4;
  fmt.compByteWidth = 1;
  fmt.compType = CompType::UNorm;
  return fmt;
}

DependencyViewer::DependencyViewer(ICaptureContext &ctx, QWidget *parent)
    : QFrame(parent), ui(new Ui::DependencyViewer), m_Ctx(ctx)
{
    ui->setupUi(this);
    setWindowTitle(tr("Dependency Viewer"));
    connect(ui->graphicsView, &DependencyView::ZoomScale, this,
            [this](QString s) { ui->label->setText(s); });
    TestFunc();
}

DependencyViewer::~DependencyViewer()
{
    m_Ctx.BuiltinWindowClosed(this);
    m_Ctx.RemoveCaptureViewer(this);
    qobject_cast<DependencyScene *>(ui->graphicsView->scene())->SetCanUpdate(false);

    delete ui;
}

void DependencyViewer::TestFunc()
{
  auto scene = qobject_cast<DependencyScene *>(ui->graphicsView->scene());

  rdcarray<DepPassInfo> passes = CreateFakeDepPassInfos();

  std::map<ResourceId, PassItem *> outputMap;
  std::vector<PassItem *> nodeItems;

  QPoint basePos(100, 100);
  int yOffset = 0;

  for(const auto &pass : passes)
  {
    // 创建一个图形节点
    PassItem *item = new PassItem();
    scene->addItem(item);
    item->setPos(basePos.x(), basePos.y() + yOffset);
    yOffset += 300;    // 每个 node 垂直偏移

    // 设置标题：Pass idx、EID、FBO、RP
    QString title = QStringLiteral("Pass %1 (%2-%3)\nFBO %4, RP %5")
                        .arg(pass.m_PassIdx)
                        .arg(pass.m_StartEid)
                        .arg(pass.m_EndEid)
                        .arg(ToStr(pass.m_FBO))
                        .arg(ToStr(pass.m_RP));
    item->mTItem->mItem->setText(title);

    // 填充 Input 信息展示
    for(const auto &input : pass.m_InputInfos)
    {
      QString usageText = QStringLiteral("Slot %1, Draw %2, Usage: %3, ImgID %4")
                              .arg(input.m_BindSlot)
                              .arg(input.m_DrawEid)
                              .arg(input.m_Usage)
                              .arg(ToStr(input.m_ImageInfo.m_ImageID));
      item->mInfos->mItems[input.m_BindSlot % item->mInfos->mItems.size()]->mItem->setText(usageText);
    }

    // 建立输出资源映射
    for(const auto &output : pass.m_OutputInfos)
    {
      outputMap[output.m_ImageInfo.m_ImageID] = item;
    }

    nodeItems.push_back(item);
  }

  // 建立连接：input 依赖哪个 output（通过 ResourceId 匹配）
  for(auto *targetItem : nodeItems)
  {
    const auto &pass =
        passes[targetItem->mTItem->mItem->text().split(QStringLiteral(" ")).at(1).toUInt()];    // 获取当前 PassId

    for(const auto &input : pass.m_InputInfos)
    {
      ResourceId res = input.m_ImageInfo.m_ImageID;

      if(outputMap.count(res) > 0)
      {
        PassItem *srcItem = outputMap[res];
        scene->AddLink(srcItem->mItem->mOut.get(), targetItem->mTItem->mIn.get());
      }
    }
  }
}

void DependencyViewer::showEvent(QShowEvent *e)
{
  static_cast<DependencyScene *>(ui->graphicsView->scene())->Update();
}

void DependencyViewer::OnCaptureLoaded()
{
}

void DependencyViewer::OnCaptureClosed()
{
}

void DependencyViewer::OnEventChanged(uint32_t eventId)
{
}