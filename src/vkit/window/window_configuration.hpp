#pragma once

#include <string>

namespace vkit::window {

class WindowConfiguration {
 public:
  bool show{true};
  bool fullscreen{false};

  glm::ivec2 size{1200, 800};
  std::string title{"vkit"};
};

}  // namespace vkit::window
