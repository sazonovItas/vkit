#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "vkit/dataformat/dataformat.hpp"
#include "vkit/graphics/device.hpp"

namespace vkit::graphics {

class Swapchain {
 public:
  explicit Swapchain(const GfxDevice& gfxDevice, const vk::SurfaceKHR& surface,
                     std::uint32_t minImageCount, glm::ivec2 size);

  auto recreate(glm::ivec2 size) -> bool;

  [[nodiscard]] auto getExtent() const -> vk::Extent2D;
  [[nodiscard]] auto getFormat() const -> dataformat::Format;

  [[nodiscard]] auto acquireNextImage(vk::Semaphore signal)
      -> std::optional<std::uint32_t>;

  [[nodiscard]] auto present(std::uint32_t imageIndex,
                             vk::Semaphore waitSemaphore) -> bool;

  [[nodiscard]] auto getImage(std::uint32_t index) const -> vk::Image;
  [[nodiscard]] auto getImageView(std::uint32_t index) const -> vk::ImageView;

  [[nodiscard]] auto imageBaseBarrier(std::uint32_t imageIndex) const
      -> vk::ImageMemoryBarrier2;

 private:
  const GfxDevice& device_;

  vk::SwapchainCreateInfoKHR ci_;
  vk::UniqueSwapchainKHR swapchain_;

  std::vector<vk::Image> images_;
  std::vector<vk::UniqueImageView> imageViews_;

  void recreateImages();
  void recreateImageViews();

  auto createSwapchainCreateInfo(const vk::SurfaceKHR& surface,
                                 std::uint32_t minImageCount)
      -> vk::SwapchainCreateInfoKHR;
};

};  // namespace vkit::graphics
