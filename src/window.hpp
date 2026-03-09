#pragma once

#include <memory>
#include <span>

#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"

namespace glfw {
struct Deleter {
  void operator()(GLFWwindow* window) const noexcept;
};

using Window = std::unique_ptr<GLFWwindow, Deleter>;

[[nodiscard]] auto createWindow(glm::ivec2 size, char const* title) -> Window;
[[nodiscard]] auto instanceExtensions() -> std::span<char const* const>;
[[nodiscard]] auto createSurface(GLFWwindow* window, vk::Instance instance)
    -> vk::UniqueSurfaceKHR;

auto framebufferSize(GLFWwindow* window) -> glm::ivec2;
}  // namespace glfw
