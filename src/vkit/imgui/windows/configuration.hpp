#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "vkit/imgui/imgui_window.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/renderer/types.hpp"

namespace vkit::imgui::windows {

}  // namespace vkit::imgui::windows

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
  void setSceneParams(renderer::types::SceneParamsUBO* sceneParams);
  void setLights(std::vector<renderer::types::Light>* lights);
  void setMaterialPreviewData(material::MaterialManager* matManager,
                              std::uint32_t* previewSlot);
  void setWorkflowController(
      controller::WorkflowController* workflowController);
  void setSelectedPrimitive(std::optional<std::uint32_t>* selectedPrimitive);
  void setCameraLight(renderer::types::Light* cameraLight,
                      bool* cameraLightEnabled);

 protected:
  void onDraw() override;

 private:
  controller::AssetController* assetController_{nullptr};
  controller::EnvironmentController* envController_{nullptr};
  controller::WorkflowController* workflowController_{nullptr};

  animation::Animator* animator_{nullptr};
  bool* enableSkinning_{nullptr};
  renderer::types::SceneParamsUBO* sceneParams_{nullptr};
  std::vector<renderer::types::Light>* lights_{nullptr};
  std::optional<std::uint32_t>* selectedPrimitive_{nullptr};
  renderer::types::Light* cameraLight_{nullptr};
  bool* cameraLightEnabled_{nullptr};

  material::MaterialManager* matManager_{nullptr};
  std::uint32_t* previewSlot_{nullptr};

  char assetNameBuffer_[256]{};
  std::optional<std::uint32_t> lastSelectedAssetId_{std::nullopt};

  char envNameBuffer_[256]{};
  std::optional<std::uint32_t> lastSelectedEnvId_{std::nullopt};

  void drawAssetManagementSection();
  void drawSceneParamsConfigurationSection();
  void drawLightingSection();
  void drawAnimationManagementSection();
  void drawPrimitivesSection();
  void drawEnvironmentManagementSection();
  void drawMaterialPreivewSection();
};

};  // namespace vkit::imgui::windows
