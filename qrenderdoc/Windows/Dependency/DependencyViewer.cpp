#include "DependencyViewer.h"
#include <QDebug>
#include <QPropertyAnimation>
#include "dependency_info.h"    // 包含 vivo::DepPassInfo 等结构
#include "ui_DependencyViewer.h"
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
  DependencyScene *scene = qobject_cast<DependencyScene *>(ui->graphicsView->scene());
  scene->clear();

  rdcarray<vivo::DepPassInfo> passes = m_Ctx.GetDepPassInfos();

  const int spacingX = 400;
  const int spacingY = 300;
  const int itemsPerRow = 4;

  int row = 0, col = 0;

  for(int i = 0; i < passes.size(); ++i)
  {
    const auto &pass = passes[i];

    PassItem *item = new PassItem();
    scene->addItem(item);

    QString title = QStringLiteral("Pass ID #%1 (%2 - %3)\nFBO %4, RP %5")
                        .arg(pass.m_PassIdx)
                        .arg(pass.m_StartEid)
                        .arg(pass.m_EndEid)
                        .arg(ToStr(pass.m_FBO))
                        .arg(ToStr(pass.m_RP));

    item->SetTitleText(title);
    item->SetInputInfoList(pass.m_InputInfos);
    item->SetOutputInfoList(pass.m_OutputInfos);

    // 自动排布
    int x = 100 + col * spacingX;
    int y = 100 + row * spacingY;
    item->setPos(x, y);

    if(++col >= itemsPerRow)
    {
      col = 0;
      ++row;
    }
  }

  ui->graphicsView->centerOn(0, 0);
}

void DependencyViewer::showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
  if(auto *scene = qobject_cast<DependencyScene *>(ui->graphicsView->scene()))
    scene->Update();
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