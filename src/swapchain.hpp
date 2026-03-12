#pragma once

#include <optional>
#include <vector>

#include "render_target.hpp"
#include "vk_mem_alloc.hpp"
#include "vku/images/allocated_image.hpp"

namespace vkit {
class Swapchain {
 public:
  explicit Swapchain(vk::Device device, vk::PhysicalDevice physical_device,
                     std::uint32_t queueFamily, vma::Allocator allocator,
                     vk::SurfaceKHR surface, glm::ivec2 size);

  auto recreate(glm::ivec2 size) -> bool;

  [[nodiscard]] auto getSize() const -> glm::ivec2 {
    return {ci_.imageExtent.width, ci_.imageExtent.height};
  }

  [[nodiscard]] auto getFormat() const -> vk::Format { return ci_.imageFormat; }

  [[nodiscard]] auto acquireNextImage(vk::Semaphore to_signal)
      -> std::optional<RenderTarget>;

  [[nodiscard]] auto baseBarrier() const -> vk::ImageMemoryBarrier2;
  [[nodiscard]] auto baseColorBarrier() const -> vk::ImageMemoryBarrier2;
  [[nodiscard]] auto baseDepthBarrier() const -> vk::ImageMemoryBarrier2;

  [[nodiscard]] auto getPresentSemaphore() const -> vk::Semaphore;
  [[nodiscard]] auto present(vk::Queue queue) -> bool;

 private:
  void populateImages();
  void createImageViews();
  void createPresentSemaphores();

  void populateColorImage();
  void populateDepthImage();

  vk::PhysicalDevice physicalDevice_;
  std::uint32_t queueFamily_;
  vk::Device device_;
  vma::Allocator allocator_;

  vk::SwapchainCreateInfoKHR ci_;
  vk::UniqueSwapchainKHR swapchain_;
  std::vector<vk::Image> images_;
  std::vector<vk::UniqueImageView> imageViews_;
  std::vector<vk::UniqueSemaphore> presentSemaphores_;
  std::optional<std::size_t> imageIdx_;

  vku::AllocatedImage colorImage_;
  vk::UniqueImageView colorImageView_;

  vku::AllocatedImage depthImage_;
  vk::UniqueImageView depthImageView_;
};

}  // namespace vkit
