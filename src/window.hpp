#pragma once
#include <memory>
#include <span>

#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"

namespace lvk::glfw {
struct Deleter {
  void operator()(GLFWwindow* window) const noexcept;
};

using Window = std::unique_ptr<GLFWwindow, Deleter>;

[[nodiscard]] auto create_window(glm::ivec2 size, char const* title) -> Window;
[[nodiscard]] auto instance_extensions() -> std::span<char const* const>;
[[nodiscard]] auto create_surface(GLFWwindow* window, vk::Instance instance)
    -> vk::UniqueSurfaceKHR;

auto framebuffer_size(GLFWwindow* window) -> glm::ivec2;
}  // namespace lvk::glfw
