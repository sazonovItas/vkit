#include "vkit/imgui/imgui_window_manager.hpp"

#include <algorithm>

namespace vkit::imgui {

void ImguiWindowManager::addWindow(std::shared_ptr<ImguiWindow> window) {
  windows_.push_back(std::move(window));
}

void ImguiWindowManager::removeWindow(const std::string& title) {
  std::ranges::remove_if(
      windows_, [&title](const auto& win) { return win->title() == title; });
}

void ImguiWindowManager::drawWindows() {
  for (auto& window : windows_) {
    window->render();
  }
}

void ImguiWindowManager::drawWindowMenu() {
  if (ImGui::BeginMenu("Windows")) {
    for (auto& window : windows_) {
      if (!window->showInMenu()) continue;

      bool is_visible = window->isVisible();
      if (ImGui::MenuItem(window->title().c_str(), nullptr, &is_visible)) {
        window->setVisibility(is_visible);
      }
    }
    ImGui::EndMenu();
  }
}

}  // namespace vkit::imgui
