#include "window.hpp"

#include <print>
#include <stdexcept>

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"

namespace glfw {
void Deleter::operator()(GLFWwindow* window) const noexcept {
  glfwDestroyWindow(window);
  glfwTerminate();
}

auto createWindow(glm::ivec2 size, const char* title) -> Window {
  static auto const kOnError = [](int const code, char const* description) {
    std::println(stderr, "[GLFW] ERROR {}: {}", code, description);
  };
  glfwSetErrorCallback(kOnError);

  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error{"failed to init GLFW"};
  }

  if (glfwVulkanSupported() != GLFW_TRUE) {
    throw std::runtime_error{"glfw vulkan is not supported"};
  }

  auto ret = Window{};
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  ret.reset(glfwCreateWindow(size.x, size.y, title, nullptr, nullptr));
  if (!ret) {
    throw std::runtime_error{"failed to create GLFW window"};
  }

  return ret;
}

auto instanceExtensions() -> std::span<char const* const> {
  auto count = std::uint32_t{};
  auto const* extensions = glfwGetRequiredInstanceExtensions(&count);
  return {extensions, static_cast<std::size_t>(count)};
}

auto createSurface(GLFWwindow* window, vk::Instance const instance)
    -> vk::UniqueSurfaceKHR {
  VkSurfaceKHR ret{};
  auto const result = glfwCreateWindowSurface(instance, window, nullptr, &ret);
  if (result != VK_SUCCESS || ret == VkSurfaceKHR{}) {
    throw std::runtime_error{"failed to create vulkan surface"};
  }

  return vk::UniqueSurfaceKHR{ret, instance};
}

auto framebufferSize(GLFWwindow* window) -> glm::ivec2 {
  auto ret = glm::ivec2{};
  glfwGetFramebufferSize(window, &ret.x, &ret.y);
  return ret;
}
}  // namespace glfw
