#include "vkit/imgui/windows/viewport.hpp"

#include <imgui.h>
#include <imgui_internal.h>

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
  const float border = ImGui::GetStyle().WindowBorderSize;
  const ImVec2 padding = ImGui::GetStyle().WindowPadding;

  const ImVec2 window_pos = ImGui::GetWindowPos();
  const ImVec2 window_size = ImGui::GetWindowSize();
  const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

  const ImVec2 p_min = ImVec2(window_pos.x + border, cursor_pos.y - padding.y);

  const ImVec2 p_max = ImVec2(window_pos.x + window_size.x - border,
                              window_pos.y + window_size.y - border);

  const ImVec2 size = ImVec2(p_max.x - p_min.x, p_max.y - p_min.y);

  if (size.x > 0 && size.y > 0 &&
      (static_cast<std::uint32_t>(size.x) != currentWidth_ ||
       static_cast<std::uint32_t>(size.y) != currentHeight_)) {
    currentWidth_ = static_cast<std::uint32_t>(size.x);
    currentHeight_ = static_cast<std::uint32_t>(size.y);
    if (onResize_) onResize_(currentWidth_, currentHeight_);
  }

  if (currentTexture_.has_value()) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const float border = window->WindowBorderSize;
    const float window_rounding = window->WindowRounding;

    const float inner_rounding = std::max(0.0F, window_rounding - border);

    const ImDrawFlags flags = ImDrawFlags_RoundCornersAll;

    draw_list->AddImageRounded(
        currentTexture_.value(), p_min, p_max, ImVec2(0, 0), ImVec2(1, 1),
        IM_COL32(255, 255, 255, 255), inner_rounding, flags);
  }

  contentRectX_ = p_min.x;
  contentRectY_ = p_min.y;
  contentRectWidth_ = size.x;
  contentRectHeight_ = size.y;
}

};  // namespace vkit::imgui::windows
