#pragma once

#include "vkit/window/window.hpp"

namespace vkit::graphics {

class Instance {
  using Window = window::Window;

 public:
  Instance();

  auto get() const -> vk::Instance { return *instance_; }

 private:
  vk::UniqueInstance instance_;
  vk::UniqueDebugUtilsMessengerEXT debugMessenger_;
};

};  // namespace vkit::graphics
