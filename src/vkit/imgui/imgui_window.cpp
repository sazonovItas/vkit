#include "vkit/imgui/imgui_window.hpp"

#include <imgui.h>

namespace vkit::imgui {

ImguiWindow::ImguiWindow(std::string_view title, bool showInMenu)
    : title_{title}, showInMenu_{showInMenu} {}

auto ImguiWindow::isVisible() const -> bool { return isVisible_; }

void ImguiWindow::setVisibility(bool visible) { isVisible_ = visible; }

void ImguiWindow::toggleVisibility() { isVisible_ = !isVisible_; }

void ImguiWindow::show() { setVisibility(true); }

void ImguiWindow::hide() { setVisibility(false); }

auto ImguiWindow::title() const -> const std::string& { return title_; }

auto ImguiWindow::showInMenu() const -> bool { return showInMenu_; }

auto ImguiWindow::isHovered() const -> bool { return isHovered_; }

void ImguiWindow::setMinSize(float minWidth, float minHeight) {
  minSize_[0] = minWidth;
  minSize_[1] = minHeight;
}

void ImguiWindow::setMaxSize(float maxWidth, float maxHeight) {
  maxSize_[0] = maxWidth;
  maxSize_[1] = maxHeight;
}

auto ImguiWindow::getFlags() -> ImGuiWindowFlags {
  return ImGuiWindowFlags_None;
}

void ImguiWindow::render() {
  if (!isVisible_) return;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{5.0F, 5.0F});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0F);

  const ImVec4 focused_color = ImVec4{0.26F, 0.59F, 0.98F, 1.0F};
  const ImVec4 default_color = ImVec4{0.15F, 0.15F, 0.15F, 1.0F};
  ImGui::PushStyleColor(ImGuiCol_Border,
                        isFocused_ ? focused_color : default_color);

  onBegin();

  ImGui::SetNextWindowSizeConstraints(ImVec2{minSize_[0], minSize_[1]},
                                      ImVec2{maxSize_[0], maxSize_[1]});

  const auto not_collapsed =
      ImGui::Begin(title_.c_str(), &isVisible_, getFlags());

  if (not_collapsed) {
    onDraw();
  }

  onEnd();

  isHovered_ =
      ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
  isFocused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

  ImGui::End();

  ImGui::PopStyleColor(1);
  ImGui::PopStyleVar(3);
}

}  // namespace vkit::imgui
