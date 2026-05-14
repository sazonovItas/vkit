#include "vkit/imgui/imgui_window.hpp"

#include <imgui.h>
#include <imgui_internal.h>

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

auto ImguiWindow::isFocused() const -> bool { return isFocused_; }

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

  ImGuiWindowClass window_class;
  window_class.ClassId = ImGui::GetID("vkit_windows");
  window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_HiddenTabBar;
  ImGui::SetNextWindowClass(&window_class);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3.0F, 3.0F));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));

  const auto not_collapsed = ImGui::Begin(
      title_.c_str(), nullptr, getFlags() | ImGuiWindowFlags_NoTitleBar);

  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar(1);

  if (not_collapsed) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0F);

    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);

    ImVec4 active_border = ImVec4(0.122F, 0.498F, 0.769F, 1.0F);
    ImVec4 inactive_border = ImGui::GetStyle().Colors[ImGuiCol_Border];
    ImGui::PushStyleColor(ImGuiCol_Border,
                          isFocused_ ? active_border : inactive_border);

    const auto child_flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoScrollbar;

    ImGui::BeginChild("##inner_panel", ImGui::GetContentRegionAvail(), 1,
                      child_flags);

    onBegin();
    onDraw();
    onEnd();

    ImGui::EndChild();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
  }

  isHovered_ =
      ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows |
                             ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
  isFocused_ = ImGui::IsWindowFocused(ImGuiHoveredFlags_RootAndChildWindows);

  ImGui::End();
}

}  // namespace vkit::imgui
