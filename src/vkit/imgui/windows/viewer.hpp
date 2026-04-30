#pragma once

#include <imgui.h>

#include <cstdint>
#include <functional>
#include <glm/vec2.hpp>
#include <optional>

#include "vkit/imgui/imgui_window.hpp"

namespace vkit::imgui::windows {

class Viewer : public ImguiWindow {
 public:
  using ResizeCallback = std::function<void(std::uint32_t, std::uint32_t)>;
  using ManipulateCallback = std::function<void(Viewer&)>;

  explicit Viewer(std::string_view name, ResizeCallback onResize = nullptr,
                  ManipulateCallback onManipulate = nullptr);

  ~Viewer() override = default;

  void setCurrentTexture(std::optional<ImTextureID> id);

  void setOnResize(ResizeCallback onResize);
  void setOnManipulate(ManipulateCallback onManipulate);

  [[nodiscard]] auto getWidth() const -> std::uint32_t;
  [[nodiscard]] auto getHeight() const -> std::uint32_t;
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
  ManipulateCallback onManipulate_;

  float contentRectX_{0.0F};
  float contentRectY_{0.0F};
  float contentRectWidth_{0.0F};
  float contentRectHeight_{0.0F};
};

};  // namespace vkit::imgui::windows
