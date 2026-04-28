#pragma once

#include <imgui.h>

#include <cstdint>
#include <functional>
#include <glm/vec2.hpp>
#include <optional>

#include "vkit/imgui/imgui_window.hpp"

namespace vkit::imgui::windows {

class ViewportWindow : public ImguiWindow {
 public:
  using ResizeCallback = std::function<void(std::uint32_t, std::uint32_t)>;

  explicit ViewportWindow(std::string_view name, ResizeCallback onResize);
  ~ViewportWindow() override = default;

  void setCurrentTexture(std::optional<ImTextureID> id);

  [[nodiscard]] auto getWidth() const -> std::uint32_t;
  [[nodiscard]] auto getHeight() const -> std::uint32_t;
  [[nodiscard]] auto isHovered() const -> bool;
  [[nodiscard]] auto toContent(glm::vec2 positionInRoot) const -> glm::vec2;

 protected:
  void onBegin() override;
  void onEnd() override;
  void onDraw() override;

 private:
  std::uint32_t currentWidth_{1280};
  std::uint32_t currentHeight_{720};
  std::optional<ImTextureID> currentTexture_{std::nullopt};
  ResizeCallback onResize_;

  float contentRectX_{0.0F};
  float contentRectY_{0.0F};
  float contentRectWidth_{0.0F};
  float contentRectHeight_{0.0F};
};

}  // namespace vkit::imgui::windows
