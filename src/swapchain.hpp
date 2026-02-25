#pragma once
#include <vk_mem_alloc.h>

#include <optional>
#include <vector>

#include "render_target.hpp"
#include "vulkan/gpu.hpp"
#include "vulkan/vma/image.hpp"

namespace lvk {

class Swapchain {
 public:
  explicit Swapchain(vk::Device device, vk::PhysicalDevice physical_device,
                     vkit::vulkan::QueueFamilies& queue_families,
                     VmaAllocator allocator, vk::SurfaceKHR surface,
                     glm::ivec2 size);

  auto recreate(glm::ivec2 size) -> bool;

  [[nodiscard]] auto get_size() const -> glm::ivec2 {
    return {m_ci_.imageExtent.width, m_ci_.imageExtent.height};
  }

  [[nodiscard]] auto get_format() const -> vk::Format {
    return m_ci_.imageFormat;
  }

  [[nodiscard]] auto acquire_next_image(vk::Semaphore to_signal)
      -> std::optional<RenderTarget>;

  [[nodiscard]] auto base_barrier() const -> vk::ImageMemoryBarrier2;
  [[nodiscard]] auto base_color_barrier() const -> vk::ImageMemoryBarrier2;
  [[nodiscard]] auto base_depth_barrier() const -> vk::ImageMemoryBarrier2;

  [[nodiscard]] auto get_present_semaphore() const -> vk::Semaphore;
  [[nodiscard]] auto present(vk::Queue queue) -> bool;

 private:
  void populate_images();
  void create_image_views();
  void create_present_semaphores();

  void populate_color_image();
  void populate_depth_image();

  vk::PhysicalDevice m_physical_device_;
  vkit::vulkan::QueueFamilies m_queue_families_;
  vk::Device m_device_;
  VmaAllocator m_allocator_;

  vk::SwapchainCreateInfoKHR m_ci_;
  vk::UniqueSwapchainKHR m_swapchain_;
  std::vector<vk::Image> m_images_;
  std::vector<vk::UniqueImageView> m_image_views_;
  std::vector<vk::UniqueSemaphore> m_present_semaphorses_;
  std::optional<std::size_t> m_image_index_;

  vkit::vulkan::vma::Image m_color_image_;
  vk::UniqueImageView m_color_image_view_;

  vkit::vulkan::vma::Image m_depth_image_;
  vk::UniqueImageView m_depth_image_view_;
};

}  // namespace lvk
