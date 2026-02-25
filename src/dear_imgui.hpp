#pragma once

#include "GLFW/glfw3.h"
#include "scoped/scoped.hpp"

namespace lvk {
struct DearImGuiCreateInfo {
  GLFWwindow* window;
  std::uint32_t api_vesrion;
  vk::Instance instance;
  vk::PhysicalDevice physical_device;
  std::uint32_t queue_family;
  vk::Device device;
  vk::Queue queue;
  vk::Format color_format;
  vk::SampleCountFlagBits samples;
};

class DearImGui {
 public:
  using CreateInfo = DearImGuiCreateInfo;

  explicit DearImGui(CreateInfo const& create_info);

  void new_frame();
  void end_frame();
  void render(vk::CommandBuffer cb) const;

 private:
  enum class State : std::int8_t { kEnded, kBegun };

  struct Deleter {
    void operator()(vk::Device device) const;
  };

  State m_state_;

  vkit::Scoped<vk::Device, Deleter> m_device_;
};
};  // namespace lvk
