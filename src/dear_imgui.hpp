#pragma once

#include "GLFW/glfw3.h"
#include "resource_buffering.hpp"
#include "vku/scoped/scoped.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit {
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

  vk::UniqueDescriptorPool descriptorPool;

  explicit DearImGui(const CreateInfo& createInfo);

  virtual void newFrame();
  virtual void endFrame();
  virtual void render(vk::CommandBuffer cb) const;

 private:
  enum class State : std::int8_t { kEnded, kBegun };

  struct Deleter {
    void operator()(vk::Device device) const;
  };

  State state_;

  vku::Scoped<vk::Device, Deleter> device_;

  auto createDescriptorPool(vk::Device device) -> vk::UniqueDescriptorPool {
    static constexpr auto kPoolSizeV = std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,
                               2 * kResourceBufferingV},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer,
                               2 * kResourceBufferingV},

        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 256}};

    auto pool_ci = vk::DescriptorPoolCreateInfo{};
    pool_ci.setPoolSizes(kPoolSizeV)
        .setMaxSets(256)
        .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind |
                  vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    return device.createDescriptorPoolUnique(pool_ci);
  }
};
};  // namespace vkit
