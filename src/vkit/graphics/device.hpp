#pragma once

#include <vk_mem_alloc.hpp>

#include "vkit/graphics/instance.hpp"
#include "vkit/graphics/surface.hpp"
#include "vkit/util/scoped.hpp"

namespace vkit::graphics {

struct QueueFamilies {
  std::uint32_t compute, graphicsPresent, transfer;
  std::vector<std::uint32_t> uniqueIndices;

  QueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
};

struct Queues {
  vk::Queue compute, graphicsPresent, transfer;

  Queues(vk::Device device, const QueueFamilies& queueFamilies) noexcept;
};

class GfxDevice {
 public:
  vk::PhysicalDevice physicalDevice;
  vk::PhysicalDeviceProperties properties;
  QueueFamilies queueFamilies;

  vk::UniqueDevice device;
  Queues queues;

  vma::Allocator allocator;

  vk::UniqueCommandPool transferCommandPool;
  vk::UniqueCommandPool graphicsPresentCommandPool;

  GfxDevice(const Instance& instance, const Surface& surface);

  auto get() const -> vk::Device { return *device; }

  auto getTransferCommandPool() const -> vk::CommandPool {
    return *transferCommandPool;
  }

  auto getGraphicsPresentCommandPool() const -> vk::CommandPool {
    return *graphicsPresentCommandPool;
  }

  auto createCommandPool(std::uint32_t queueFamilyIndex,
                         vk::CommandPoolCreateFlagBits flags =
                             vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
      -> vk::UniqueCommandPool;

  void waitIdle() { device->waitIdle(); }

  ~GfxDevice() { allocator.destroy(); };

 private:
  [[nodiscard]] static auto selectGpu(const vk::Instance& instance,
                                      const vk::SurfaceKHR& surface)
      -> vk::PhysicalDevice;

  [[nodiscard]] auto createDevice() -> vk::UniqueDevice;
  [[nodiscard]] auto createAllocator(const vk::Instance& instance) const
      -> vma::Allocator;
};

struct DeviceWaiterDeleter {
  void operator()(vk::Device const device) const noexcept { device.waitIdle(); }
};

using DeviceWaiter = vkit::util::Scoped<vk::Device, DeviceWaiterDeleter>;

};  // namespace vkit::graphics
