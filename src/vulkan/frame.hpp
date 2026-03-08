#pragma once

namespace vkit::vulkan {
struct Frame {
 public:
  vk::UniqueSemaphore renderingFinishSema;
  vk::UniqueSemaphore swapchainImageAcquireSema;
  vk::UniqueFence inFlightFence;
};
};  // namespace vkit::vulkan
