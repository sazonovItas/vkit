#pragma once

#include <memory>
#include <vector>

#include "vkit/imgui/imgui_window.hpp"

namespace vkit::imgui {

class ImguiWindowManager {
 public:
  ImguiWindowManager() = default;
  ~ImguiWindowManager() = default;

  void addWindow(std::shared_ptr<ImguiWindow> window);
  void removeWindow(const std::string& title);

  void drawWindows();

  void drawWindowMenu();

 private:
  std::vector<std::shared_ptr<ImguiWindow>> windows_;
};

}  // namespace vkit::imgui
