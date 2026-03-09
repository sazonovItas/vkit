#pragma once

#include "GLFW/glfw3.h"
#include "vku/scoped/scoped.hpp"

namespace lvk {
struct DearImGuiCreateInfo {
  GLFWwindow* window;
  std::uint32_t apiVersion;
  vk::Instance instance;
  vk::PhysicalDevice physicalDevice;
  std::uint32_t queueFamily;
  vk::Device device;
  vk::Queue queue;
  vk::Format colorFormat;
  vk::SampleCountFlagBits samples;
};

class DearImGui {
 public:
  using CreateInfo = DearImGuiCreateInfo;

  explicit DearImGui(const CreateInfo& createInfo);

  void newFrame();
  void endFrame();
  void render(vk::CommandBuffer cb) const;

 private:
  enum class State : std::int8_t { kEnded, kBegun };

  struct Deleter {
    void operator()(vk::Device device) const;
  };

  State state_;

  vku::Scoped<vk::Device, Deleter> device_;
};
};  // namespace lvk
