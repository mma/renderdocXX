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

rdcarray<DepPassInfo> CreateFakeDepPassInfos()
{
  rdcarray<DepPassInfo> fakePasses;

  // 模拟资源 ID
  ResourceId image1 = ResourceId(1001);
  ResourceId image2 = ResourceId(1002);
  ResourceId image3 = ResourceId(1003);
  ResourceId cmdBuf1 = ResourceId(2001);
  ResourceId rp1 = ResourceId(3001);
  ResourceId fbo1 = ResourceId(4001);

  //Pass 0: RenderPass 写 image1 和 image2
  {
    DepPassInfo pass;
    pass.m_PassIdx = 0;
    pass.m_StartEid = 10;
    pass.m_EndEid = 20;
    pass.m_PassType = PASS_TYPE_RENDERPASS;
    pass.m_RP = rp1;
    pass.m_FBO = fbo1;
    pass.m_DrawEid = {12, 14};

    // 输出 image1
    DepOutputInfo output1;
    output1.m_ImageInfo = DepImageInfo(image1, {0});    // mip 0
    output1.m_Format = MakeRGBA8Format();
    output1.m_Width = 256;
    output1.m_Height = 256;
    output1.m_Depth = 1;
    output1.m_LoadOp = LOAD_OP_CLEAR;
    output1.m_StoreOp = STORE_OP_STORE;
    output1.m_StencilLoadOp = LOAD_OP_DONT_CARE;
    output1.m_StencilStoreOp = STORE_OP_DONT_CARE;

    // 输出 image2
    DepOutputInfo output2 = output1;
    output2.m_ImageInfo = DepImageInfo(image2, {0});    // mip 0

    pass.m_OutputInfos.push_back(output1);
    pass.m_OutputInfos.push_back(output2);

    fakePasses.push_back(pass);
  }

  // Pass 1: ComputePass 写 image3
  {
    DepPassInfo pass;
    pass.m_PassIdx = 1;
    pass.m_StartEid = 30;
    pass.m_EndEid = 30;
    pass.m_PassType = PASS_TYPE_COMPUTE;
    pass.m_DrawEid = {30};

    DepOutputInfo output;
    output.m_ImageInfo = DepImageInfo(image3, {0});
    output.m_Format = MakeRGBA8Format();
    output.m_Width = 256;
    output.m_Height = 256;
    output.m_Depth = 1;
    output.m_LoadOp = LOAD_OP_DONT_CARE;
    output.m_StoreOp = STORE_OP_STORE;
    output.m_StencilLoadOp = LOAD_OP_DONT_CARE;
    output.m_StencilStoreOp = STORE_OP_DONT_CARE;

    pass.m_OutputInfos.push_back(output);

    fakePasses.push_back(pass);
  }

  // Pass 2: RenderPass 采样 image1 和 image3
  {
    DepPassInfo pass;
    pass.m_PassIdx = 2;
    pass.m_StartEid = 40;
    pass.m_EndEid = 50;
    pass.m_PassType = PASS_TYPE_RENDERPASS;
    pass.m_RP = ResourceId(3002);
    pass.m_FBO = ResourceId(4002);
    pass.m_DrawEid = {42, 45};

    // 输入 image1
    DepInputInfo input1;
    input1.m_ImageInfo = DepImageInfo(image1, {0});
    input1.m_BindSlot = 0;
    input1.m_DrawEid = 42;
    input1.m_Usage = "Sample";

    // 输入 image3
    DepInputInfo input2 = input1;
    input2.m_ImageInfo = DepImageInfo(image3, {0});
    input2.m_BindSlot = 1;
    input2.m_DrawEid = 45;

    pass.m_InputInfos.push_back(input1);
    pass.m_InputInfos.push_back(input2);

    fakePasses.push_back(pass);
  }

  return fakePasses;
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