#pragma once

#include <optional>
#include <string_view>

#include "vkit/imgui/imgui_window.hpp"
#include "vkit/material/manager.hpp"

namespace vkit::controller {
class AssetController;
class EnvironmentController;
class WorkflowController;
};  // namespace vkit::controller

namespace vkit::animation {
class Animator;
};

namespace vkit::imgui::windows {

class ConfigurationWindow : public ImguiWindow {
 public:
  explicit ConfigurationWindow(
      std::string_view title,
      controller::AssetController* assetController = nullptr,
      controller::EnvironmentController* envController = nullptr,
      animation::Animator* animator = nullptr, bool* enableSkinning = nullptr,
      material::MaterialManager* matManager = nullptr,
      std::uint32_t* previewSlot = nullptr);

  ~ConfigurationWindow() override = default;

  void setAssetController(controller::AssetController* assetController);
  void setEnvironmentController(
      controller::EnvironmentController* envController);
  void setAnimator(animation::Animator* animator);
  void setEnableSkinning(bool* enableSkinning);
  void setMaterialPreviewData(material::MaterialManager* matManager,
                              std::uint32_t* previewSlot);
  void setWorkflowController(
      controller::WorkflowController* workflowController);

 protected:
  void onDraw() override;

 private:
  controller::AssetController* assetController_{nullptr};
  controller::EnvironmentController* envController_{nullptr};
  controller::WorkflowController* workflowController_{nullptr};

  animation::Animator* animator_{nullptr};
  bool* enableSkinning_{nullptr};

  material::MaterialManager* matManager_{nullptr};
  std::uint32_t* previewSlot_{nullptr};

  char assetNameBuffer_[256]{};
  std::optional<std::uint32_t> lastSelectedAssetId_{std::nullopt};

  char envNameBuffer_[256]{};
  std::optional<std::uint32_t> lastSelectedEnvId_{std::nullopt};

  void drawAssetManagementSection();
  void drawAnimationManagementSection();
  void drawPrimitiveMaterialSection();
  void drawEnvironmentManagementSection();
  void drawMaterialPreivewSection();
};

};  // namespace vkit::imgui::windows
