#pragma once

#include <optional>
#include <string_view>

#include "vkit/imgui/imgui_window.hpp"

namespace vkit::controller {
class AssetController;
class EnvironmentController;
};  // namespace vkit::controller

namespace vkit::imgui::windows {

class ConfigurationWindow : public ImguiWindow {
 public:
  explicit ConfigurationWindow(
      std::string_view title,
      controller::AssetController* assetController = nullptr,
      controller::EnvironmentController* envController = nullptr);

  ~ConfigurationWindow() override = default;

  void setAssetController(controller::AssetController* assetController);
  void setEnvironmentController(
      controller::EnvironmentController* envController);

 protected:
  void onDraw() override;

 private:
  controller::AssetController* assetController_{nullptr};
  controller::EnvironmentController* envController_{nullptr};

  char assetNameBuffer_[256]{};
  std::optional<std::uint32_t> lastSelectedAssetId_{std::nullopt};

  char envNameBuffer_[256]{};
  std::optional<std::uint32_t> lastSelectedEnvId_{std::nullopt};

  void drawAssetManagementSection();
  void drawEnvironmentManagementSection();
};

};  // namespace vkit::imgui::windows
