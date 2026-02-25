#pragma once

#include <vk_mem_alloc.h>

#include "scoped/scoped.hpp"
#include "vulkan/util.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit::vulkan::vma {
struct RawImage {
  auto operator==(RawImage const& rhs) const -> bool = default;

  VmaAllocator allocator{};
  VmaAllocation allocation{};
  vk::Image image;
  vk::Extent2D extent;
  vk::Format format{};
  std::uint32_t levels{};
};

struct ImageDeleter {
  void operator()(RawImage const& raw_image) const noexcept;
};

using Image = Scoped<RawImage, ImageDeleter>;

struct ImageCreateInfo {
  std::uint32_t levels{1};
  vk::SampleCountFlagBits sample_count{vk::SampleCountFlagBits::e1};
  VmaAllocator allocator;
  std::uint32_t queue_family;
};

[[nodiscard]] auto create_image(ImageCreateInfo const& create_info,
                                vk::ImageUsageFlags usage, vk::Format format,
                                vk::Extent2D extent) -> Image;

struct Bitmap {
  std::span<std::byte const> bytes;
  glm::ivec2 size{};
};

[[nodiscard]] auto create_sampled_image(ImageCreateInfo const& create_info,
                                        util::CommandBlock command_block,
                                        Bitmap const& bitmap) -> Image;
};  // namespace vkit::vulkan::vma
