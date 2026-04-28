#pragma once

#include <imgui.h>

#include <string>
#include <string_view>

namespace vkit::imgui {

class ImguiWindow {
 public:
  explicit ImguiWindow(std::string_view title, bool showInMenu = true);
  virtual ~ImguiWindow() = default;

  virtual void onDraw() = 0;

  virtual void onBegin() {}
  virtual void onEnd() {}

  [[nodiscard]] auto isVisible() const -> bool;
  void setVisibility(bool visible);
  void toggleVisibility();
  void show();
  void hide();

  [[nodiscard]] auto title() const -> const std::string&;
  [[nodiscard]] auto showInMenu() const -> bool;
  [[nodiscard]] auto isHovered() const -> bool;
  [[nodiscard]] auto isFocused() const -> bool;

  void setMinSize(float minWidth, float minHeight);
  void setMaxSize(float maxWidth, float maxHeight);
  virtual auto getFlags() -> ImGuiWindowFlags;

  void render();

 protected:
  std::string title_;
  bool showInMenu_{true};

  float minSize_[2]{100.0F, 100.0F};
  float maxSize_[2]{99999.0F, 99999.0F};

 private:
  bool isVisible_{true};
  bool isHovered_{false};
  bool isFocused_{false};
};

}  // namespace vkit::imgui
