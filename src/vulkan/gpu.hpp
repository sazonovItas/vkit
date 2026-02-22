#pragma once

#include "vma/allocator.hpp"

namespace vkit::vulkan {
struct QueueFamilies {
  std::uint32_t compute, graphics_present, transfer;
  std::vector<std::uint32_t> unique_indices;

  QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);
};

struct Queues {
  vk::Queue compute, graphics_present, transfer;

  Queues(vk::Device device, const QueueFamilies& queue_families) noexcept;
};

class Gpu {
 public:
  vk::PhysicalDevice physical_device;
  vk::PhysicalDeviceProperties physical_device_properties;
  QueueFamilies queue_families;
  vk::UniqueDevice device;
  Queues queues;
  vma::Allocator allocator;

  Gpu(const vk::Instance& instance, vk::SurfaceKHR surface);

 private:
  [[nodiscard]] auto select_gpu(const vk::Instance& instance,
                                vk::SurfaceKHR surface) const
      -> vk::PhysicalDevice;
  [[nodiscard]] auto create_device() -> vk::UniqueDevice;
  [[nodiscard]] auto create_allocator(const vk::Instance& instance) const
      -> vma::Allocator;
};
};  // namespace vkit::vulkan
