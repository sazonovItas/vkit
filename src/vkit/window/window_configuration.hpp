#pragma once

#include <string>

namespace vkit::window {

class WindowConfiguration {
 public:
  bool show{true};
  bool fullscreen{false};
  bool highPixelDensity{false};

  int swapInterval{1};

  vk::Extent2D size{};
  std::string title{"vkit"};
};

}  // namespace vkit::window
