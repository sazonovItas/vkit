#pragma once

#include <optional>
#include <string_view>

#include "vkit/imgui/imgui_window.hpp"

namespace vkit::controller {
class AssetController;
}

namespace vkit::imgui::windows {

class ConfigurationWindow : public ImguiWindow {
 public:
  explicit ConfigurationWindow(
      std::string_view title,
      controller::AssetController* assetController = nullptr);

  ~ConfigurationWindow() override = default;

  void setAssetController(controller::AssetController* assetController);

  auto getFlags() -> ImGuiWindowFlags override;

 protected:
  void onDraw() override;

 private:
  controller::AssetController* assetController_{nullptr};

  char assetNameBuffer_[256]{};
  std::optional<std::uint32_t> lastSelectedAssetId_{std::nullopt};

  void drawAssetManagementSection();
};

};  // namespace vkit::imgui::windows
