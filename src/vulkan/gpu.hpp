#pragma once

#include "vk_mem_alloc.hpp"

namespace vkit::vulkan {
struct QueueFamilies {
  std::uint32_t compute, graphicsPresent, transfer;
  std::vector<std::uint32_t> uniqueIndices;

  QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);
};

struct Queues {
  vk::Queue compute, graphicsPresent, transfer;

  Queues(vk::Device device, const QueueFamilies& queue_families) noexcept;
};

class Gpu {
 public:
  vk::PhysicalDevice physicalDevice;
  vk::PhysicalDeviceProperties properties;
  QueueFamilies queueFamilies;

  vk::UniqueDevice device;
  Queues queues;

  vma::Allocator allocator;

  Gpu(const vk::Instance& instance, vk::SurfaceKHR surface);

  auto createCommandPool(std::uint32_t queueFamilyIndex,
                         vk::CommandPoolCreateFlagBits flags =
                             vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
      -> vk::UniqueCommandPool;

  ~Gpu() { allocator.destroy(); };

 private:
  [[nodiscard]] auto selectGpu(const vk::Instance& instance,
                               vk::SurfaceKHR surface) const
      -> vk::PhysicalDevice;
  [[nodiscard]] auto createDevice() -> vk::UniqueDevice;
  [[nodiscard]] auto createAllocator(const vk::Instance& instance) const
      -> vma::Allocator;
};
};  // namespace vkit::vulkan
