#pragma once

#include "vkit/graphics/instance.hpp"
#include "vkit/window/window.hpp"

namespace vkit::graphics {

class Surface {
  using Window = window::Window;

 public:
  explicit Surface(const Window& window, const Instance& instance);

  [[nodiscard]] auto get() const -> vk::SurfaceKHR { return *surface_; }

 private:
  vk::UniqueSurfaceKHR surface_;
};

};  // namespace vkit::graphics
