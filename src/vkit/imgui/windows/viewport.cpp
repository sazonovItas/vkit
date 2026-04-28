#include "vkit/imgui/windows/viewport.hpp"

#include <imgui.h>

namespace vkit::imgui::windows {

ViewportWindow::ViewportWindow(std::string_view name, ResizeCallback onResize)
    : ImguiWindow(name, true), onResize_(std::move(onResize)) {}

void ViewportWindow::setCurrentTexture(std::optional<ImTextureID> id) {
  currentTexture_ = id;
}

auto ViewportWindow::getWidth() const -> std::uint32_t { return currentWidth_; }

auto ViewportWindow::getHeight() const -> std::uint32_t {
  return currentHeight_;
}

auto ViewportWindow::toContent(glm::vec2 positionInRoot) const -> glm::vec2 {
  const float content_x = positionInRoot.x - contentRectX_;
  const float content_y = positionInRoot.y - contentRectY_;
  return glm::vec2{content_x, content_y};
}

void ViewportWindow::onBegin() {}

void ViewportWindow::onEnd() {}

void ViewportWindow::onDraw() {
  const ImVec2 size = ImGui::GetContentRegionAvail();

  if (size.x > 0 && size.y > 0 &&
      (static_cast<std::uint32_t>(size.x) != currentWidth_ ||
       static_cast<std::uint32_t>(size.y) != currentHeight_)) {
    currentWidth_ = static_cast<std::uint32_t>(size.x);
    currentHeight_ = static_cast<std::uint32_t>(size.y);
    if (onResize_) onResize_(currentWidth_, currentHeight_);
  }

  if (currentTexture_.has_value()) {
    ImGui::Image(currentTexture_.value(), size, ImVec2(0, 0), ImVec2(1, 1));
  }

  const ImVec2 rect_min = ImGui::GetItemRectMin();
  const ImVec2 rect_max = ImGui::GetItemRectMax();
  contentRectX_ = rect_min.x;
  contentRectY_ = rect_min.y;
  contentRectWidth_ = rect_max.x - rect_min.x;
  contentRectHeight_ = rect_max.y - rect_min.y;
}

}  // namespace vkit::imgui::windows
