#pragma once

#include <QFrame>
#include <QWidget>
#include <memory>
#include <vector>
#include "Code/Interface/QRDInterface.h"
#include "Code/QRDUtils.h"
#include "ComposeWidget.h"
#include "DependencyScene.h"
#include "DependencyView.h"
#include "FlagNameItem.h"
#include "PassItem.h"

namespace Ui
{
class DependencyViewer;
}

class ComposeWidget;
class QGraphicsLayoutItem;
class DependencyScene;
class DependencyView;
class FlagNameItem;

class DependencyViewer : public QFrame, public IDependencyViewer, public ICaptureViewer
{
  Q_OBJECT

public:
  explicit DependencyViewer(ICaptureContext &ctx, QWidget *parent = 0);
  ~DependencyViewer();

  QWidget *Widget() override { return this; }

  // ICaptureViewer
  void OnCaptureLoaded() override;
  void OnCaptureClosed() override;
  void OnSelectedEventChanged(uint32_t eventId) override {}
  void OnEventChanged(uint32_t eventId) override;

  void TestFunc();
  void showEvent(QShowEvent *e) override;

private:
  Ui::DependencyViewer *ui;
  std::shared_ptr<ComposeWidget> mCompose;
  std::shared_ptr<ComposeWidget> mSubCompose;
  // 图形布局容器
  ComposeWidget *m_ComposeWidget = nullptr;

  // 图形视图组件（包含 QGraphicsScene）
  DependencyView *m_DependencyView = nullptr;

  ICaptureContext &m_Ctx;

  uint32_t m_Pass;
  std::vector<ResourceId> m_Resources;
};
